import os
import sys
import urllib
import urllib.request
import time
import numpy as np
import argparse
import cv2,math
from math import ceil
from itertools import product as product
from shapely.geometry import Polygon

from rknn.api import RKNN

CLASSES = ['plane', 'ship', 'storage tank', 'baseball diamond', 'tennis court', 
'basketball court', 'ground track field', 'harbor', 'bridge', 'large vehicle', 'small vehicle', 'helicopter',
           'roundabout', 'soccer ball field', 'swimming pool']

nmsThresh = 0.4
objectThresh = 0.5

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
    def __init__(self, classId, score, xmin, ymin, xmax, ymax,angle):
        self.classId = classId
        self.score = score
        self.xmin = xmin
        self.ymin = ymin
        self.xmax = xmax
        self.ymax = ymax
        self.angle=angle

def rotate_rectangle(x1, y1, x2, y2, a):
    # 计算中心点坐标
    cx = (x1 + x2) / 2
    cy = (y1 + y2) / 2

    # 将角度转换为弧度
    # a = math.radians(a)
    # 对每个顶点进行旋转变换
    x1_new = int((x1 - cx) * math.cos(a) - (y1 - cy) * math.sin(a) + cx)
    y1_new = int((x1 - cx) * math.sin(a) + (y1 - cy) * math.cos(a) + cy)

    x2_new = int((x2 - cx) * math.cos(a) - (y2 - cy) * math.sin(a) + cx)
    y2_new = int((x2 - cx) * math.sin(a) + (y2 - cy) * math.cos(a) + cy)

    x3_new = int((x1 - cx) * math.cos(a) - (y2 - cy) * math.sin(a) + cx)
    y3_new = int((x1 - cx) * math.sin(a) + (y2 - cy) * math.cos(a) + cy)

    x4_new =int( (x2 - cx) * math.cos(a) - (y1 - cy) * math.sin(a) + cx)
    y4_new =int( (x2 - cx) * math.sin(a) + (y1 - cy) * math.cos(a) + cy)
    return [(x1_new, y1_new), (x3_new, y3_new),(x2_new, y2_new) ,(x4_new, y4_new)]



def intersection(g, p):
    g=np.asarray(g)
    p=np.asarray(p)
    g = Polygon(g[:8].reshape((4, 2)))
    p = Polygon(p[:8].reshape((4, 2)))
    if not g.is_valid or not p.is_valid:
        return 0
    inter = Polygon(g).intersection(Polygon(p)).area
    union = g.area + p.area - inter
    if union == 0:
        return 0
    else:
        return inter/union

def NMS(detectResult):
    predBoxs = []

    sort_detectboxs = sorted(detectResult, key=lambda x: x.score, reverse=True)
    for i in range(len(sort_detectboxs)):
        xmin1 = sort_detectboxs[i].xmin
        ymin1 = sort_detectboxs[i].ymin
        xmax1 = sort_detectboxs[i].xmax
        ymax1 = sort_detectboxs[i].ymax
        classId = sort_detectboxs[i].classId
        angle = sort_detectboxs[i].angle
        p1=rotate_rectangle(xmin1, ymin1, xmax1, ymax1, angle)
        p1=np.array(p1).reshape(-1)
        
        if sort_detectboxs[i].classId != -1:
            predBoxs.append(sort_detectboxs[i])
            for j in range(i + 1, len(sort_detectboxs), 1):
                if classId == sort_detectboxs[j].classId:
                    xmin2 = sort_detectboxs[j].xmin
                    ymin2 = sort_detectboxs[j].ymin
                    xmax2 = sort_detectboxs[j].xmax
                    ymax2 = sort_detectboxs[j].ymax
                    angle2 = sort_detectboxs[j].angle
                    p2=rotate_rectangle(xmin2, ymin2, xmax2, ymax2, angle2)
                    p2=np.array(p2).reshape(-1)
                    iou=intersection(p1, p2)
                    if iou > nmsThresh:
                        sort_detectboxs[j].classId = -1
    return predBoxs

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def softmax(x, axis=-1):
    # 将输入向量减去最大值以提高数值稳定性
    exp_x = np.exp(x - np.max(x, axis=axis, keepdims=True))
    return exp_x / np.sum(exp_x, axis=axis, keepdims=True)



def process(out,model_w,model_h,stride,angle_feature,index,scale_w=1,scale_h=1):
    class_num=len(CLASSES)
    angle_feature=angle_feature.reshape(-1)
    xywh=out[:,:64,:]
    conf=sigmoid(out[:,64:,:])
    out=[]
    conf=conf.reshape(-1)
    for ik in range(model_h*model_w*class_num):
        if conf[ik]>objectThresh:
            w=ik%model_w
            h=(ik%(model_w*model_h))//model_w
            c=ik//(model_w*model_h)
            xywh_=xywh[0,:,(h*model_w)+w] #[1,64,1]
            xywh_=xywh_.reshape(1,4,16,1)
            data=np.array([i for i in range(16)]).reshape(1,1,16,1)
            xywh_=softmax(xywh_,2)
            xywh_ = np.multiply(data, xywh_)
            xywh_ = np.sum(xywh_, axis=2, keepdims=True).reshape(-1)
            xywh_add=xywh_[:2]+xywh_[2:]
            xywh_sub=(xywh_[2:]-xywh_[:2])/2
            angle_feature_= (angle_feature[index+(h*model_w)+w]-0.25)*3.1415927410125732
            angle_feature_cos=math.cos(angle_feature_)
            angle_feature_sin=math.sin(angle_feature_)
            xy_mul1=xywh_sub[0] * angle_feature_cos
            xy_mul2=xywh_sub[1] * angle_feature_sin
            xy_mul3=xywh_sub[0] * angle_feature_sin
            xy_mul4=xywh_sub[1] * angle_feature_cos
            xy=xy_mul1-xy_mul2,xy_mul3+xy_mul4
            xywh_1=np.array([(xy_mul1-xy_mul2)+w+0.5,(xy_mul3+xy_mul4)+h+0.5,xywh_add[0],xywh_add[1]])
            xywh_=xywh_1*stride
            xmin = (xywh_[0] - xywh_[2] / 2) * scale_w
            ymin = (xywh_[1] - xywh_[3] / 2) * scale_h
            xmax = (xywh_[0] + xywh_[2] / 2) * scale_w
            ymax = (xywh_[1] + xywh_[3] / 2) * scale_h
            box = DetectBox(c,conf[ik], xmin, ymin, xmax, ymax,angle_feature_)
            out.append(box)
    return out


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='RetinaFace Python Demo', add_help=True)
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
    img = cv2.imread('../model/test.jpg')

    letterbox_img, aspect_ratio, offset_x, offset_y = letterbox_resize(img, (640,640), 114)  # letterbox缩放
    infer_img = letterbox_img[..., ::-1]  # BGR2RGB

    # Inference
    print('--> Running model')
    results = rknn.inference(inputs=[infer_img])

    outputs=[]
    for x in results[:-1]:
        index,stride=0,0
        if x.shape[2]==20:
            stride=32
            index=20*4*20*4+20*2*20*2
        if x.shape[2]==40:
            stride=16
            index=20*4*20*4
        if x.shape[2]==80:
            stride=8
            index=0
        feature=x.reshape(1,79,-1)
        output=process(feature,x.shape[3],x.shape[2],stride,results[-1],index)
        outputs=outputs+output
    predbox = NMS(outputs)

    for index in range(len(predbox)):
        xmin = int((predbox[index].xmin-offset_x)/aspect_ratio)
        ymin = int((predbox[index].ymin-offset_y)/aspect_ratio)
        xmax = int((predbox[index].xmax-offset_x)/aspect_ratio)
        ymax = int((predbox[index].ymax-offset_y)/aspect_ratio)
        classId = predbox[index].classId
        score = predbox[index].score
        angle = predbox[index].angle
        points=rotate_rectangle(xmin,ymin,xmax,ymax,angle)
        cv2.polylines(img, [np.asarray(points, dtype=int)], True,  (0, 255, 0), 1)

        ptext = (xmin, ymin)
        title= CLASSES[classId] + "%.2f" % score
        cv2.putText(img, title, ptext, cv2.FONT_HERSHEY_DUPLEX, 0.5, (0, 0, 255), 1)
    img_path = './result.jpg'
    cv2.imwrite(img_path, img)
    print("save image in", img_path)
    # Release
    rknn.release()
