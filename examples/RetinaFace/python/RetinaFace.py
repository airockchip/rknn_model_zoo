import os
import sys
import urllib
import urllib.request
import time
import numpy as np
import cv2
from math import ceil
from itertools import product as product

from rknn.api import RKNN
DATASET_PATH = '../model/dataset.txt'
DEFAULT_RKNN_PATH = '../model/RetinaFace.rknn'
DEFAULT_QUANT = True

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

    # 计算调整后的图像尺寸
    aspect_ratio = min(target_width / image_width, target_height / image_height)
    new_width = int(image_width * aspect_ratio)
    new_height = int(image_height * aspect_ratio)

    # 使用 cv2.resize() 进行等比缩放
    image = cv2.resize(image, (new_width, new_height), interpolation=cv2.INTER_AREA)

    # 创建新的画布并进行填充
    result_image = np.ones((target_height, target_width, 3), dtype=np.uint8) * bg_color
    offset_x = (target_width - new_width) // 2
    offset_y = (target_height - new_height) // 2
    result_image[offset_y:offset_y + new_height, offset_x:offset_x + new_width] = image
    return result_image, aspect_ratio, offset_x, offset_y

def PriorBox(image_size): #image_size Support (320,320) and (640,640)
    anchors = []
    min_sizes = [[16, 32], [64, 128], [256, 512]]
    steps = [8, 16, 32]
    feature_maps = [[ceil(image_size[0] / step), ceil(image_size[1] / step)] for step in steps]
    for k, f in enumerate(feature_maps):
        min_sizes_ = min_sizes[k]
        for i, j in product(range(f[0]), range(f[1])):
            for min_size in min_sizes_:
                s_kx = min_size / image_size[1]
                s_ky = min_size / image_size[0]
                dense_cx = [x * steps[k] / image_size[1] for x in [j + 0.5]]
                dense_cy = [y * steps[k] / image_size[0] for y in [i + 0.5]]
                for cy, cx in product(dense_cy, dense_cx):
                    anchors += [cx, cy, s_kx, s_ky]
    output = np.array(anchors).reshape(-1, 4)
    print("image_size:",image_size," num_priors=",output.shape[0])
    return output


def box_decode(loc, priors):
    """Decode locations from predictions using priors to undo
    the encoding we did for offset regression at train time.
    Args:
        loc (tensor): location predictions for loc layers,
            Shape: [num_priors,4]
        priors (tensor): Prior boxes in center-offset form.
            Shape: [num_priors,4].
        variances: (list[float]) Variances of priorboxes
    Return:
        decoded bounding box predictions
    """
    variances = [0.1, 0.2]
    boxes = np.concatenate((
        priors[:, :2] + loc[:, :2] * variances[0] * priors[:, 2:],
        priors[:, 2:] * np.exp(loc[:, 2:] * variances[1])), axis=1)
    boxes[:, :2] -= boxes[:, 2:] / 2
    boxes[:, 2:] += boxes[:, :2]
    return boxes


def decode_landm(pre, priors):
    """Decode landm from predictions using priors to undo
    the encoding we did for offset regression at train time.
    Args:
        pre (tensor): landm predictions for loc layers,
            Shape: [num_priors,10]
        priors (tensor): Prior boxes in center-offset form.
            Shape: [num_priors,4].
        variances: (list[float]) Variances of priorboxes
    Return:
        decoded landm predictions
    """
    variances = [0.1, 0.2]
    landmarks = np.concatenate((
        priors[:, :2] + pre[:, :2] * variances[0] * priors[:, 2:],
        priors[:, :2] + pre[:, 2:4] * variances[0] * priors[:, 2:],
        priors[:, :2] + pre[:, 4:6] * variances[0] * priors[:, 2:],
        priors[:, :2] + pre[:, 6:8] * variances[0] * priors[:, 2:],
        priors[:, :2] + pre[:, 8:10] * variances[0] * priors[:, 2:]
    ), axis=1)
    return landmarks


def nms(dets, thresh):
    """Pure Python NMS baseline."""
    x1 = dets[:, 0]
    y1 = dets[:, 1]
    x2 = dets[:, 2]
    y2 = dets[:, 3]
    scores = dets[:, 4]

    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        xx1 = np.maximum(x1[i], x1[order[1:]])
        yy1 = np.maximum(y1[i], y1[order[1:]])
        xx2 = np.minimum(x2[i], x2[order[1:]])
        yy2 = np.minimum(y2[i], y2[order[1:]])

        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h
        ovr = inter / (areas[i] + areas[order[1:]] - inter)

        inds = np.where(ovr <= thresh)[0]
        order = order[inds + 1]

    return keep

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]));
        print("       platform choose from [rk3562,rk3566,rk3568,rk3588]")
        print("       dtype choose from    [i8, fp]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type == 'i8':
            do_quant = True
        else:
            do_quant = False

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = DEFAULT_RKNN_PATH

    return model_path, platform, do_quant, output_path

if __name__ == '__main__':
    model_path, platform, do_quant, output_path = parse_arg()
    # Create RKNN object
    rknn = RKNN()

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[[104, 117, 123]], std_values=[[1, 1, 1]], target_platform=platform,
                quantized_algorithm="normal", quant_img_RGB2BGR=True)  # mmse
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=model_path)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=do_quant, dataset=DATASET_PATH)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')

    # Set inputs
    img = cv2.imread('../model/test.jpg')
    img_height, img_width, _ = img.shape
    model_height, model_width = (320, 320)
    letterbox_img, aspect_ratio, offset_x, offset_y = letterbox_resize(img, (model_height,model_width), 114)  # letterbox缩放
    infer_img = letterbox_img[..., ::-1]  # BGR2RGB

    # Init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')

    # Inference
    print('--> Running model')
    outputs = rknn.inference(inputs=[infer_img])
    loc, conf, landmarks = outputs
    priors = PriorBox(image_size=(model_height, model_width))
    boxes = box_decode(loc.squeeze(0), priors)
    scale = np.array([model_width, model_height,
                      model_width, model_height])
    boxes = boxes * scale // 1  # face box
    boxes[...,0::2] =np.clip((boxes[...,0::2] - offset_x) / aspect_ratio, 0, img_width)  #letterbox
    boxes[...,1::2] =np.clip((boxes[...,1::2] - offset_y) / aspect_ratio, 0, img_height) #letterbox
    scores = conf.squeeze(0)[:, 1]  # face score
    landmarks = decode_landm(landmarks.squeeze(
        0), priors)  # face keypoint data
    scale_landmarks = np.array([model_width, model_height, model_width, model_height,
                                model_width, model_height, model_width, model_height,
                                model_width, model_height])
    landmarks = landmarks * scale_landmarks // 1
    landmarks[...,0::2] = np.clip((landmarks[...,0::2] - offset_x) / aspect_ratio, 0, img_width) #letterbox
    landmarks[...,1::2] = np.clip((landmarks[...,1::2] - offset_y) / aspect_ratio, 0, img_height) #letterbox
    # ignore low scores
    inds = np.where(scores > 0.02)[0]
    boxes = boxes[inds]
    landmarks = landmarks[inds]
    scores = scores[inds]

    order = scores.argsort()[::-1]
    boxes = boxes[order]
    landmarks = landmarks[order]
    scores = scores[order]

    # NMS
    dets = np.hstack((boxes, scores[:, np.newaxis])).astype(
        np.float32, copy=False)
    keep = nms(dets, 0.5)
    dets = dets[keep, :]
    landmarks = landmarks[keep]
    dets = np.concatenate((dets, landmarks), axis=1)

    for data in dets:
        if data[4] < 0.2:
            continue
        print("face @ (%d %d %d %d) %f"%(data[0], data[1], data[2], data[3], data[4]))
        text = "{:.4f}".format(data[4])
        data = list(map(int, data))
        cv2.rectangle(img, (data[0], data[1]),
                      (data[2], data[3]), (0, 0, 255), 2)
        cx = data[0]
        cy = data[1] + 12
        cv2.putText(img, text, (cx, cy),
                    cv2.FONT_HERSHEY_DUPLEX, 0.5, (255, 255, 255))
        # landmarks
        cv2.circle(img, (data[5], data[6]), 1, (0, 0, 255), 5)
        cv2.circle(img, (data[7], data[8]), 1, (0, 255, 255), 5)
        cv2.circle(img, (data[9], data[10]), 1, (255, 0, 255), 5)
        cv2.circle(img, (data[11], data[12]), 1, (0, 255, 0), 5)
        cv2.circle(img, (data[13], data[14]), 1, (255, 0, 0), 5)
    img_path = './result.jpg'
    cv2.imwrite(img_path, img)
    print("save image in", img_path)
    # Release
    rknn.release()
