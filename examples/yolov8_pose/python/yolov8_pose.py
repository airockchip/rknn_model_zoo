import os
import sys
import urllib
import urllib.request
import time
import numpy as np
import argparse
import cv2,math
from math import ceil

from rknn.api import RKNN

CLASSES = ['person']

nmsThresh = 0.4
objectThresh = 0.5
image_size = 640

def letterbox_resize(image, size, bg_color):
    """
    letterbox_resize the image according to the specified size
    :param image: input image, which can be a NumPy array or file path
    :param size: target size (width, height)
    :param bg_color: background filling data 
    :return: processed image
    """
    if isinstance(image, str):
        image = cv2.imread(image)

    target_width, target_height = size
    image_height, image_width, _ = image.shape

    # Calculate the adjusted image size
    aspect_ratio = min(target_width / image_width, target_height / image_height)
    new_width = int(image_width * aspect_ratio)
    new_height = int(image_height * aspect_ratio)

    # Use cv2.resize() for proportional scaling
    image = cv2.resize(image, (new_width, new_height), interpolation=cv2.INTER_AREA)

    # Create a new canvas and fill it
    result_image = np.ones((target_height, target_width, 3), dtype=np.uint8) * bg_color
    offset_x = (target_width - new_width) // 2
    offset_y = (target_height - new_height) // 2
    result_image[offset_y:offset_y + new_height, offset_x:offset_x + new_width] = image
    return result_image, aspect_ratio, offset_x, offset_y


class DetectBox:
    def __init__(self, classId, score, xmin, ymin, xmax, ymax, keypoint):
        self.classId = classId
        self.score = score
        self.xmin = xmin
        self.ymin = ymin
        self.xmax = xmax
        self.ymax = ymax
        self.keypoint = keypoint

def IOU(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2):
    xmin = max(xmin1, xmin2)
    ymin = max(ymin1, ymin2)
    xmax = min(xmax1, xmax2)
    ymax = min(ymax1, ymax2)

    innerWidth = xmax - xmin
    innerHeight = ymax - ymin

    innerWidth = innerWidth if innerWidth > 0 else 0
    innerHeight = innerHeight if innerHeight > 0 else 0

    innerArea = innerWidth * innerHeight

    area1 = (xmax1 - xmin1) * (ymax1 - ymin1)
    area2 = (xmax2 - xmin2) * (ymax2 - ymin2)

    total = area1 + area2 - innerArea

    return innerArea / total


def NMS(detectResult):
    predBoxs = []

    sort_detectboxs = sorted(detectResult, key=lambda x: x.score, reverse=True)

    for i in range(len(sort_detectboxs)):
        xmin1 = sort_detectboxs[i].xmin
        ymin1 = sort_detectboxs[i].ymin
        xmax1 = sort_detectboxs[i].xmax
        ymax1 = sort_detectboxs[i].ymax
        classId = sort_detectboxs[i].classId

        if sort_detectboxs[i].classId != -1:
            predBoxs.append(sort_detectboxs[i])
            for j in range(i + 1, len(sort_detectboxs), 1):
                if classId == sort_detectboxs[j].classId:
                    xmin2 = sort_detectboxs[j].xmin
                    ymin2 = sort_detectboxs[j].ymin
                    xmax2 = sort_detectboxs[j].xmax
                    ymax2 = sort_detectboxs[j].ymax
                    iou = IOU(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2)
                    if iou > nmsThresh:
                        sort_detectboxs[j].classId = -1
    return predBoxs


def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def softmax(x, axis=-1):
    # 将输入向量减去最大值以提高数值稳定性
    exp_x = np.exp(x - np.max(x, axis=axis, keepdims=True))
    return exp_x / np.sum(exp_x, axis=axis, keepdims=True)

def process(out,keypoints,index,model_w,model_h,stride,scale_w=1,scale_h=1):
    xywh=out[:,:64,:]
    conf=sigmoid(out[:,64:,:])
    out=[]
    for h in range(model_h):
        for w in range(model_w):
            for c in range(len(CLASSES)):
                if conf[0,c,(h*model_w)+w]>objectThresh:
                    xywh_=xywh[0,:,(h*model_w)+w] #[1,64,1]
                    xywh_=xywh_.reshape(1,4,16,1)
                    data=np.array([i for i in range(16)]).reshape(1,1,16,1)
                    xywh_=softmax(xywh_,2)
                    xywh_ = np.multiply(data, xywh_)
                    xywh_ = np.sum(xywh_, axis=2, keepdims=True).reshape(-1)

                    xywh_temp=xywh_.copy()
                    xywh_temp[0]=(w+0.5)-xywh_[0]
                    xywh_temp[1]=(h+0.5)-xywh_[1]
                    xywh_temp[2]=(w+0.5)+xywh_[2]
                    xywh_temp[3]=(h+0.5)+xywh_[3]

                    xywh_[0]=((xywh_temp[0]+xywh_temp[2])/2)
                    xywh_[1]=((xywh_temp[1]+xywh_temp[3])/2)
                    xywh_[2]=(xywh_temp[2]-xywh_temp[0])
                    xywh_[3]=(xywh_temp[3]-xywh_temp[1])
                    xywh_=xywh_*stride

                    xmin=(xywh_[0] - xywh_[2] / 2) * scale_w
                    ymin = (xywh_[1] - xywh_[3] / 2) * scale_h
                    xmax = (xywh_[0] + xywh_[2] / 2) * scale_w
                    ymax = (xywh_[1] + xywh_[3] / 2) * scale_h
                    keypoint=keypoints[...,(h*model_w)+w+index] 
                    keypoint[...,0:2]=keypoint[...,0:2]//1
                    box = DetectBox(c,conf[0,c,(h*model_w)+w], xmin, ymin, xmax, ymax,keypoint)
                    out.append(box)

    return out

def check_output_consistence(output: list):
    try:
        assert len(output) == 4, (
            f"Expected 4 output tensors, got {len(output)}"
        )
        for i, tensor in enumerate(output):
            assert len(tensor.shape) == 4, (
                    f"Expected 4 axis shape tensor, got {tensor.shape}"
            )

            shape = tensor.shape

            assert shape[0] == 1, (
                    f"Supports only 1 batch size in output tensor, got {shape[0]}"
            )

            # Detections feature maps
            if i < 3:
                assert shape[1] == 65, (
                    f"Second axis must match the value 65, got {shape[1]}."
                )
                assert shape[2] == shape[3], (
                    f"Supports only equal height==width feature map, got {shape[2]} and {shape[3]} shapes"
                )
            # Keypoint feature map
            else:
                assert shape[2] == 3, (
                    f"Expected each keypoint channel contains (x, y, conf) in keypoints tensor, got {shape[2]}"
                ) 
    except Exception as e:
        print(f"Found Yolo Pose model output problem: {e}")
        print(
            (
                "In general case: expected shape [(1, 65, X1, X1), (1, 65, X2, X2), (1, 65, X3, X3), (1, X4, 3, X5)]"
                f", got {[result.shape for result in output]}"
                "Make sure that converted model is from ultralytics_yolov8"
            )
        )

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Yolov8 Pose Python Demo', add_help=True)
    # basic params
    parser.add_argument('--model_path', type=str, required=True,
                        help='model path, could be .rknn file')
    parser.add_argument('--target', type=str,
                        default='rk3566', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str,
                        default=None, help='device id')
    args = parser.parse_args()

    # Create RKNN object
    rknn = RKNN(verbose=True)

    # Load RKNN model
    ret = rknn.load_rknn(args.model_path)
    if ret != 0:
        print('Load RKNN model \"{}\" failed!'.format(args.model_path))
        exit(ret)
    print('done')

    # Init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime(target=args.target, device_id=args.device_id)
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')

    # Set inputs
    img = cv2.imread('../model/bus.jpg')

    letterbox_img, aspect_ratio, offset_x, offset_y = letterbox_resize(img, (image_size, image_size), 56)  # letterbox缩放
    infer_img = letterbox_img[..., ::-1]  # BGR2RGB
    infer_img = infer_img.reshape((1, *infer_img.shape)) # Makes 4 dim instead of 3
    
    # Inference
    print('--> Running model')
    results = rknn.inference(inputs=[infer_img])

    check_output_consistence(results)

    outputs=[]
    keypoints=results[3]
    for i, x in enumerate(results[:3]):
        stride = int(image_size / x.shape[2])

        if i == 0:
            index = 0 # This is the first feature map so the start index is 0
        elif i == 1:
            index = results[i - 1].shape[2] * results[i - 1].shape[3] # index = previous_feature_map_height * previous_feature_map_width
        elif i == 2:
            index = (results[i - 2].shape[2] * results[i - 2].shape[3] +
                     results[i - 1].shape[2] * results[i - 1].shape[3])

        feature = x.reshape(1, 65, -1)
        output = process(feature, keypoints, index,
                            x.shape[3], x.shape[2], stride)
        outputs += output
    predbox = NMS(outputs)

    for i in range(len(predbox)):
        xmin = int((predbox[i].xmin-offset_x)/aspect_ratio)
        ymin = int((predbox[i].ymin-offset_y)/aspect_ratio)
        xmax = int((predbox[i].xmax-offset_x)/aspect_ratio)
        ymax = int((predbox[i].ymax-offset_y)/aspect_ratio)
        classId = predbox[i].classId
        score = predbox[i].score
        cv2.rectangle(img, (xmin, ymin), (xmax, ymax), (0, 255, 0), 2)
        ptext = (xmin, ymin)
        title= CLASSES[classId] + "%.2f" % score

        cv2.putText(img, title, ptext, cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2, cv2.LINE_AA)
        keypoints =predbox[i].keypoint.reshape(-1, 3) # keypoint [x, y, conf]
        keypoints[...,0]=(keypoints[...,0]-offset_x)/aspect_ratio
        keypoints[...,1]=(keypoints[...,1]-offset_y)/aspect_ratio

        for j, keypoint in enumerate(keypoints):
            x, y, conf = int(keypoint[0]), int(keypoint[1]), keypoint[2]
            cv2.circle(img, (x, y), 3, (0, 0, 255), -1)
            # cv2.putText(img, str(f"{j}-th: {conf:.2f}"), (x + 5, y - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 1)

    cv2.imwrite("./result.jpg", img)
    print("save image in ./result.jpg")
    # Release
    rknn.release()


