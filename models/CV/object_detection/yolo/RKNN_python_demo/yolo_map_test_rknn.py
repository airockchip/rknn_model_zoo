import os
import cv2
import sys
import argparse

from utils.coco_utils import COCO_test_helper
import numpy as np

# add path
realpath = os.path.abspath(__file__)
print(realpath)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('rknn_model_zoo')+1]))

NPU_1 = ['RK3399PRO','RK1808','RV1126','RV1109']
NPU_2 = ['RK3566','RK3568','RK3588','RK1106']

OBJ_THRESH = 0.25
NMS_THRESH = 0.45
# OBJ_THRESH = 0.001
# NMS_THRESH = 0.65

IMG_SIZE = (640, 640)  # (width, height), such as (1280, 736)
# IMG_SIZE = (768, 384)

CLASSES = ("person", "bicycle", "car","motorbike ","aeroplane ","bus ","train","truck ","boat","traffic light",
           "fire hydrant","stop sign ","parking meter","bench","bird","cat","dog ","horse ","sheep","cow","elephant",
           "bear","zebra ","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite",
           "baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife ",
           "spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza ","donut","cake","chair","sofa",
           "pottedplant","bed","diningtable","toilet ","tvmonitor","laptop	","mouse	","remote ","keyboard ","cell phone","microwave ",
           "oven ","toaster","sink","refrigerator ","book","clock","vase","scissors ","teddy bear ","hair drier", "toothbrush ")


coco_id_list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 31, 32, 33, 34,
         35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
         64, 65, 67, 70, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 89, 90]


CLASSESCOCO = ('__background__', 'person', 'bicycle', 'car', 'motorbike', 'aeroplane', 'bus', 'train', 'truck', 'boat',
           'traffic light', 'fire hydrant', '???', 'stop sign', 'parking meter', 'bench', 'bird', 'cat', 'dog', 'horse',
           'sheep', 'cow', 'elephant', 'bear', 'zebra', 'giraffe', '???', 'backpack', 'umbrella', '???', '???',
           'handbag', 'tie', 'suitcase', 'frisbee', 'skis', 'snowboard', 'sports ball', 'kite', 'baseball bat',
           'baseball glove', 'skateboard', 'surfboard', 'tennis racket', 'bottle', '???', 'wine glass', 'cup', 'fork',
           'knife', 'spoon', 'bowl', 'banana', 'apple', 'sandwich', 'orange', 'broccoli', 'carrot', 'hot dog', 'pizza',
           'donut', 'cake', 'chair', 'sofa', 'pottedplant', 'bed', '???', 'diningtable', '???', '???', 'toilet',
           '???', 'tvmonitor', 'laptop', 'mouse', 'remote', 'keyboard', 'cell phone', 'microwave', 'oven', 'toaster', 'sink',
           'refrigerator', '???', 'book', 'clock', 'vase', 'scissors', 'teddy bear', 'hair drier', 'toothbrush')


def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def xywh2xyxy(x):
    # Convert [x, y, w, h] to [x1, y1, x2, y2]
    y = np.copy(x)
    y[:, 0] = x[:, 0] - x[:, 2] / 2  # top left x
    y[:, 1] = x[:, 1] - x[:, 3] / 2  # top left y
    y[:, 2] = x[:, 0] + x[:, 2] / 2  # bottom right x
    y[:, 3] = x[:, 1] + x[:, 3] / 2  # bottom right y
    return y

def process(inputs, anchors, args):
    # inputs = sigmoid(inputs)
    grid_h, grid_w = map(int, inputs.shape[0:2])
    col = np.tile(np.arange(0, grid_w), grid_h).reshape(-1, grid_w)
    row = np.tile(np.arange(0, grid_h).reshape(-1, 1), grid_w)
    col = col.reshape(grid_h, grid_w, 1, 1).repeat(len(anchors), axis=-2)
    row = row.reshape(grid_h, grid_w, 1, 1).repeat(len(anchors), axis=-2)
    grid = np.concatenate((col, row), axis=-1)

    box_confidence = inputs[..., 4]
    box_confidence = np.expand_dims(box_confidence, axis=-1)
    box_class_probs = inputs[..., 5:]

    if args.model == 'yolox':
        box_xy = inputs[..., :2]
        box_wh = np.exp(inputs[..., 2:4]) * (int(IMG_SIZE[1]/grid_h), int(IMG_SIZE[0]/grid_w))
    else:
        box_xy = inputs[..., :2]*2 - 0.5
        box_wh = pow(inputs[..., 2:4]*2, 2)

    box_xy += grid
    box_xy *= (int(IMG_SIZE[1]/grid_h), int(IMG_SIZE[0]/grid_w))
    box_wh = box_wh * anchors

    box = np.concatenate((box_xy, box_wh), axis=-1)

    return box, box_confidence, box_class_probs

def filter_boxes(boxes, box_confidences, box_class_probs, args):
    """Filter boxes with object threshold.

    # Arguments
        boxes: ndarray, boxes of objects.
        box_confidences: ndarray, confidences of objects.
        box_class_probs: ndarray, class_probs of objects.

    # Returns
        boxes: ndarray, filtered boxes.
        classes: ndarray, classes for boxes.
        scores: ndarray, scores for boxes.
    """
    boxes = boxes.reshape(-1, 4)
    box_confidences = box_confidences.reshape(-1)
    box_class_probs = box_class_probs.reshape(-1, box_class_probs.shape[-1])
    if args.model in ['yolov5', 'yolov7']:
        # filter box_confidences
        _box_pos = np.where(box_confidences >= OBJ_THRESH)
        boxes = boxes[_box_pos]
        box_confidences = box_confidences[_box_pos]
        box_class_probs = box_class_probs[_box_pos]

        class_max_score = np.max(box_class_probs, axis=-1)
        classes = np.argmax(box_class_probs, axis=-1)
        _class_pos = np.where(class_max_score* box_confidences >= OBJ_THRESH)

        boxes = boxes[_class_pos]
        classes = classes[_class_pos]
        scores = (class_max_score* box_confidences)[_class_pos]

    elif args.model == 'yolox':
        box_scores = box_confidences.reshape(-1, 1) * box_class_probs
        box_classes = np.argmax(box_scores, axis=-1)
        box_class_scores = np.max(box_scores, axis=-1)
        pos = np.where(box_class_scores >= OBJ_THRESH)

        boxes = boxes[pos]
        classes = box_classes[pos]
        scores = box_class_scores[pos]

    return boxes, classes, scores

def nms_boxes(boxes, scores):
    """Suppress non-maximal boxes.

    # Arguments
        boxes: ndarray, boxes of objects.
        scores: ndarray, scores of objects.

    # Returns
        keep: ndarray, index of effective boxes.
    """
    x = boxes[:, 0]
    y = boxes[:, 1]
    w = boxes[:, 2] - boxes[:, 0]
    h = boxes[:, 3] - boxes[:, 1]

    areas = w * h
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)

        xx1 = np.maximum(x[i], x[order[1:]])
        yy1 = np.maximum(y[i], y[order[1:]])
        xx2 = np.minimum(x[i] + w[i], x[order[1:]] + w[order[1:]])
        yy2 = np.minimum(y[i] + h[i], y[order[1:]] + h[order[1:]])

        w1 = np.maximum(0.0, xx2 - xx1 + 0.00001)
        h1 = np.maximum(0.0, yy2 - yy1 + 0.00001)
        inter = w1 * h1

        ovr = inter / (areas[i] + areas[order[1:]] - inter)
        inds = np.where(ovr <= NMS_THRESH)[0]
        order = order[inds + 1]
    keep = np.array(keep)
    return keep


def yolov5_post_process(input_data, anchors, args):
    boxes, classes, scores = [], [], []
    for _input,_an in zip(input_data, anchors):
        b, c, s = process(_input, _an, args)
        b, c, s = filter_boxes(b, c, s, args)
        boxes.append(b)
        classes.append(c)
        scores.append(s)

    boxes = np.concatenate(boxes)
    boxes = xywh2xyxy(boxes)
    classes = np.concatenate(classes)
    scores = np.concatenate(scores)

    nboxes, nclasses, nscores = [], [], []

    if args.class_agnostic:
        keep = nms_boxes(boxes, scores)
        if len(keep) != 0:
            nboxes.append(boxes[keep])
            nclasses.append(classes[keep])
            nscores.append(scores[keep])
    else:
        for c in set(classes):
            inds = np.where(classes == c)
            b = boxes[inds]
            c = classes[inds]
            s = scores[inds]
            keep = nms_boxes(b, s)

            if len(keep) != 0:
                nboxes.append(b[keep])
                nclasses.append(c[keep])
                nscores.append(s[keep])

    if not nclasses and not nscores:
        return None, None, None

    boxes = np.concatenate(nboxes)
    classes = np.concatenate(nclasses)
    scores = np.concatenate(nscores)

    return boxes, classes, scores


def draw(image, boxes, scores, classes):
    """Draw the boxes on the image.

    # Argument:
        image: original image.
        boxes: ndarray, boxes of objects.
        classes: ndarray, classes of objects.
        scores: ndarray, scores of objects.
        all_classes: all classes name.
    """
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = box
        print('class: {}, score: {}'.format(CLASSES[cl], score))
        print('box coordinate left,top,right,down: [{}, {}, {}, {}]'.format(top, left, right, bottom))
        top = int(top)
        left = int(left)
        right = int(right)
        bottom = int(bottom)

        cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6, (0, 0, 255), 2)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')
    # basic params
    parser.add_argument('--model_path', type=str, required= True, help='model path, could be .pt or .rknn file')
    parser.add_argument('--img_show', action='store_true', default=False, help='draw the result and show')
    parser.add_argument('--target', type=str, default='rk1808', help='target platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')

    # data params
    parser.add_argument('--anno_json', type=str, default='../../../../../datasets/COCO/annotations/instances_val2017.json', help='coco annotation path')
    parser.add_argument('--img_folder', type=str, default='../../../../../datasets/COCO//val2017', help='img folder path')
    parser.add_argument('--coco_map_test', action='store_true', help='enable coco map test')

    # optional according for yolov5/yolov7/yolox
    parser.add_argument('--anchors', type=str, default='anchors_yolov5.txt', help='target to anchor file')
    parser.add_argument('--std', type=int, default=255, help='std using for preprocess')
    parser.add_argument('--class_agnostic', type=bool, default=False, help='nms on all class when set False')
    parser.add_argument('--color', type=str, default='RGB', help='model input color')

    # determine model type
    parser.add_argument('--model', type=str, default='yolov5', help='model input color')

    args = parser.parse_args()

    if args.model not in ['yolov5', 'yolov7', 'yolox']:
        print('ERROR: {} model type is not support.'.format(args.model))
        exit()

    if args.model == 'yolox':
        args.anchors = None
        args.std = 1
        args.class_agnostic = True
        if args.model_path.endswith('.rknn'):
            args.color = 'RGB'
        else:
            args.color = 'BGR'

    if args.anchors is None or args.anchors =='None':
        print("None anchors file determine, free anchors for use")
        anchors = [[[1.0,1.0]]]*3
    else:
        with open(args.anchors, 'r') as f:
            values = [float(_v) for _v in f.readlines()]
            anchors = np.array(values).reshape(3,-1,2).tolist()
    print("Testing '{}' model, use anchors from '{}', which is {}".format(args.model, args.anchors, anchors))
    print('\nContinue? [Y/N]')
    key_in = ' '
    while key_in not in ['Y', 'N']:
        key_in = input("Y or N:")
    if key_in == 'N':
        exit()
    print('\n\n')
    
    # init model
    model_path = args.model_path
    if model_path.endswith('.pt') or model_path.endswith('.torchscript'):
        platform = 'pytorch'
        from common.framework_excuter.pytorch_excute import Torch_model_container
        model = Torch_model_container(args.model_path)
    elif model_path.endswith('.rknn'):
        platform = 'rknn'
        from common.framework_excuter.rknn_excute import RKNN_model_container 
        model = RKNN_model_container(args.model_path, args.target, args.device_id)
    elif model_path.endswith('onnx'):
        platform = 'onnx'
        from common.framework_excuter.onnx_excute import ONNX_model_container
        model = ONNX_model_container(args.model_path)
    else:
        assert False, "{} is not rknn/pytorch/onnx model".format(model_path)
    print('Model-{} is {} model, starting val'.format(model_path, platform))

    img_list = os.listdir(args.img_folder)
    co_helper = COCO_test_helper(enable_letter_box=True)

    # run test
    for i in range(len(img_list)):
        print('finish {}/{}'.format(i+1, len(img_list)), end='\r')

        img_name = img_list[i]
        img_src = cv2.imread(os.path.join(args.img_folder, img_name))

        '''
        # using for test input dumped by C.demo
        img_src = np.fromfile('./input_b/demo_c_input_hwc_rgb.txt', dtype=np.uint8).reshape(640,640,3)
        img_src = cv2.cvtColor(img_src, cv2.COLOR_RGB2BGR)
        '''

        # # Due to rga init with (0,0,0), we using pad_color (0,0,0) instead of (114, 114, 114)
        pad_color = (0,0,0)
        img = co_helper.letter_box(im= img_src.copy(), new_shape=(IMG_SIZE[1], IMG_SIZE[0]), pad_color=(0,0,0))

        if args.color == 'RGB':
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        elif args.color == 'BGR':
            pass
        else:
            print('{} color is not support'.format(args.color))
            exit()

        # preprocee if not rknn model
        if platform in ['pytorch', 'onnx']:
            input_data = img.transpose((2,0,1))
            input_data = input_data.reshape(1,*input_data.shape).astype(np.float32)
            input_data = input_data/args.std
        else:
            input_data = img

        outputs = model.run([input_data])
        # proprocess result
        outputs = [output.reshape([len(anchors[0]),-1]+list(output.shape[-2:])) for output in outputs]
        outputs = [np.transpose(output, (2,3,0,1)) for output in outputs]

        boxes, classes, scores = yolov5_post_process(outputs, anchors, args)

        if args.img_show is True:
            print('\n\nIMG: {}'.format(img_name))
            # if args.color == 'RGB' or (model_path.endswith('rknn') and args.target.upper() in NPU_1):
            if args.color == 'RGB':
                img_p = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
            else:
                img_p = img.copy()
            if boxes is not None:
                draw(img_p, boxes, scores, classes)
            cv2.imshow("full post process result", img_p)
            cv2.waitKeyEx(0)

        if args.coco_map_test is True:
            if boxes is not None:
                for i in range(boxes.shape[0]):
                    co_helper.add_single_record(image_id = int(img_name.split('.')[0]),
                                                category_id = coco_id_list[int(classes[i])],
                                                bbox = boxes[i],
                                                score = round(scores[i], 5).astype(np.float)
                                                )
    if args.coco_map_test is True:
        pred_json = model_path.split('.')[-2]+ '_{}'.format(platform) +'.json'
        pred_json = pred_json.split('/')[-1]
        pred_json = os.path.join('./', pred_json)
        co_helper.export_to_json(pred_json)

        from utils.coco_utils import coco_eval_with_json
        coco_eval_with_json(args.anno_json, pred_json)


