from copy import copy
import os
import cv2
import numpy as np
import json

class Letter_Box_Info():
    def __init__(self, shape, new_shape, w_ratio, h_ratio, dw, dh, pad_color) -> None:
        self.origin_shape = shape
        self.new_shape = new_shape
        self.w_ratio = w_ratio
        self.h_ratio = h_ratio
        self.dw = dw 
        self.dh = dh
        self.pad_color = pad_color


def coco_eval_with_json(anno_json, pred_json):
    from pycocotools.coco import COCO
    from pycocotools.cocoeval import COCOeval
    anno = COCO(anno_json)
    pred = anno.loadRes(pred_json)
    eval = COCOeval(anno, pred, 'bbox')
    # eval.params.useCats = 0
    # eval.params.maxDets = list((100, 300, 1000))
    # a = np.array(list(range(50, 96, 1)))/100
    # eval.params.iouThrs = a
    eval.evaluate()
    eval.accumulate()
    eval.summarize()
    map, map50 = eval.stats[:2]  # update results (mAP@0.5:0.95, mAP@0.5)

    print('map  --> ', map)
    print('map50--> ', map50)
    print('map75--> ', eval.stats[2])
    print('map85--> ', eval.stats[-2])
    print('map95--> ', eval.stats[-1])

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

        # Scale ratio
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
            self.letter_box_info_list.append(Letter_Box_Info(shape, new_shape, ratio, ratio, dw, dh, pad_color))
        if info_need is True:
            return im, ratio, (dw, dh)
        else:
            return im

    def direct_resize(self, im, new_shape, info_need=False):
        shape = im.shape[:2]
        h_ratio = new_shape[0]/ shape[0]
        w_ratio = new_shape[1]/ shape[1]
        if self.enable_ltter_box is True:
            self.letter_box_info_list.append(Letter_Box_Info(shape, new_shape, w_ratio, h_ratio, 0, 0, (0,0,0)))
        im = cv2.resize(im, (new_shape[1], new_shape[0]))
        return im

    def get_real_box(self, box, in_format='xyxy'):
        bbox = copy(box)
        if self.enable_ltter_box == True:
        # unletter_box result
            if in_format=='xyxy':
                bbox[:,0] -= self.letter_box_info_list[-1].dw
                bbox[:,0] /= self.letter_box_info_list[-1].w_ratio
                bbox[:,0] = np.clip(bbox[:,0], 0, self.letter_box_info_list[-1].origin_shape[1])

                bbox[:,1] -= self.letter_box_info_list[-1].dh
                bbox[:,1] /= self.letter_box_info_list[-1].h_ratio
                bbox[:,1] = np.clip(bbox[:,1], 0, self.letter_box_info_list[-1].origin_shape[0])

                bbox[:,2] -= self.letter_box_info_list[-1].dw
                bbox[:,2] /= self.letter_box_info_list[-1].w_ratio
                bbox[:,2] = np.clip(bbox[:,2], 0, self.letter_box_info_list[-1].origin_shape[1])

                bbox[:,3] -= self.letter_box_info_list[-1].dh
                bbox[:,3] /= self.letter_box_info_list[-1].h_ratio
                bbox[:,3] = np.clip(bbox[:,3], 0, self.letter_box_info_list[-1].origin_shape[0])
        return bbox

    def get_real_seg(self, seg):
        #! fix side effect
        dh = int(self.letter_box_info_list[-1].dh)
        dw = int(self.letter_box_info_list[-1].dw)
        origin_shape = self.letter_box_info_list[-1].origin_shape
        new_shape = self.letter_box_info_list[-1].new_shape
        if (dh == 0) and (dw == 0) and origin_shape == new_shape:
            return seg
        elif dh == 0 and dw != 0:
            seg = seg[:, :, dw:-dw] # a[0:-0] = []
        elif dw == 0 and dh != 0 : 
            seg = seg[:, dh:-dh, :]
        seg = np.where(seg, 1, 0).astype(np.uint8).transpose(1,2,0)
        seg = cv2.resize(seg, (origin_shape[1], origin_shape[0]), interpolation=cv2.INTER_LINEAR)
        if len(seg.shape) < 3:
            return seg[None,:,:]
        else:
            return seg.transpose(2,0,1)

    def add_single_record(self, image_id, category_id, bbox, score, in_format='xyxy', pred_masks = None):
        if self.enable_ltter_box == True:
        # unletter_box result
            if in_format=='xyxy':
                bbox[0] -= self.letter_box_info_list[-1].dw
                bbox[0] /= self.letter_box_info_list[-1].w_ratio

                bbox[1] -= self.letter_box_info_list[-1].dh
                bbox[1] /= self.letter_box_info_list[-1].h_ratio

                bbox[2] -= self.letter_box_info_list[-1].dw
                bbox[2] /= self.letter_box_info_list[-1].w_ratio

                bbox[3] -= self.letter_box_info_list[-1].dh
                bbox[3] /= self.letter_box_info_list[-1].h_ratio
                # bbox = [value/self.letter_box_info_list[-1].ratio for value in bbox]

        if in_format=='xyxy':
        # change xyxy to xywh
            bbox[2] = bbox[2] - bbox[0]
            bbox[3] = bbox[3] - bbox[1]
        else:
            assert False, "now only support xyxy format, please add code to support others format"
        
        def single_encode(x):
            from pycocotools.mask import encode
            rle = encode(np.asarray(x[:, :, None], order="F", dtype="uint8"))[0]
            rle["counts"] = rle["counts"].decode("utf-8")
            return rle

        if pred_masks is None:
            self.record_list.append({"image_id": image_id,
                                    "category_id": category_id,
                                    "bbox":[round(x, 3) for x in bbox],
                                    'score': round(score, 5),
                                    })
        else:
            rles = single_encode(pred_masks)
            self.record_list.append({"image_id": image_id,
                                    "category_id": category_id,
                                    "bbox":[round(x, 3) for x in bbox],
                                    'score': round(score, 5),
                                    'segmentation': rles,
                                    })
    
    def export_to_json(self, path):
        with open(path, 'w') as f:
            json.dump(self.record_list, f)

