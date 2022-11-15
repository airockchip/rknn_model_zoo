import os
import cv2
import numpy as np
import json

class Letter_Box_Info():
    def __init__(self, shape, new_shape, ratio, dw, dh, pad_color) -> None:
        self.origin_shape = shape
        self.new_shape = new_shape
        self.ratio = ratio
        self.dw = dw 
        self.dh = dh
        self.pad_color = pad_color


def coco_eval_with_json(anno_json, pred_json):
    from pycocotools.coco import COCO
    from pycocotools.cocoeval import COCOeval
    anno = COCO(anno_json)
    pred = anno.loadRes(pred_json)
    eval = COCOeval(anno, pred, 'bbox')
    eval.evaluate()
    eval.accumulate()
    eval.summarize()
    map, map50 = eval.stats[:2]  # update results (mAP@0.5:0.95, mAP@0.5)

    print('map  --> ', map)
    print('map50--> ', map50)

class COCO_test_helper():
    def __init__(self, enable_letter_box = False) -> None:
        self.record_list = []
        self.enable_ltter_box = enable_letter_box
        if self.enable_ltter_box is True:
            self.letter_box_info_list = []
        else:
            self.letter_box_info_list = None

    def letter_box(self, im, new_shape, pad_color=(0,0,0), info_need=False):
        # Resize and pad image while meeting stride-multiple constraints
        shape = im.shape[:2]  # current shape [height, width]
        if isinstance(new_shape, int):
            new_shape = (new_shape, new_shape)

        # Scale ratio (new / old)
        r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])

        # Compute padding
        ratio = r  # width, height ratios
        new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
        dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]  # wh padding

        dw /= 2  # divide padding into 2 sides
        dh /= 2

        if shape[::-1] != new_unpad:  # resize
            im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)
        top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
        left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
        im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=pad_color)  # add border
        
        if self.enable_ltter_box is True:
            self.letter_box_info_list.append(Letter_Box_Info(shape, new_shape, ratio, dw, dh, pad_color))
        if info_need is True:
            return im, ratio, (dw, dh)
        else:
            return im

    def add_single_record(self, image_id, category_id, bbox, score, in_format='xyxy'):
        if self.enable_ltter_box == True:
        # unletter_box result
            if in_format=='xyxy':
                bbox[0] -= self.letter_box_info_list[-1].dw
                bbox[1] -= self.letter_box_info_list[-1].dh
                bbox[2] -= self.letter_box_info_list[-1].dw
                bbox[3] -= self.letter_box_info_list[-1].dh
                bbox = [value/self.letter_box_info_list[-1].ratio for value in bbox]

        if in_format=='xyxy':
        # change xyxy to xywh
            bbox[2] = bbox[2] - bbox[0]
            bbox[3] = bbox[3] - bbox[1]
        else:
            assert False, "now only support xyxy format, please add code to support others format"

        self.record_list.append({"image_id": image_id,
                                 "category_id": category_id,
                                 "bbox":[round(x, 3) for x in bbox],
                                 'score': round(score, 5),
                                 })
    
    def export_to_json(self, path):
        with open(path, 'w') as f:
            json.dump(self.record_list, f)

