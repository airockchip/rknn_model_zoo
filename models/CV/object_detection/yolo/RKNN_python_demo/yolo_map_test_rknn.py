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

EX_SUPPORT_MAP = {
    'v5': 'yolov5',
    'v6': 'yolov6',
    'v7': 'yolov7',
    'v8': 'yolov8',
    'ppyoloe': 'ppyoloe_plus',
}

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


def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def filter_boxes(boxes, box_confidences, box_class_probs, args):
    """Filter boxes with object threshold.
    """
    box_confidences = box_confidences.reshape(-1)
    candidate, class_num = box_class_probs.shape

    class_max_score = np.max(box_class_probs, axis=-1)
    classes = np.argmax(box_class_probs, axis=-1)

    if args.model == 'yolov7' and class_num==1:
        _class_pos = np.where(box_confidences >= OBJ_THRESH)
        scores = (box_confidences)[_class_pos]
    else:
        _class_pos = np.where(class_max_score* box_confidences >= OBJ_THRESH)
        scores = (class_max_score* box_confidences)[_class_pos]

    boxes = boxes[_class_pos]
    classes = classes[_class_pos]

    return boxes, classes, scores

def nms_boxes(boxes, scores):
    """Suppress non-maximal boxes.
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

def dfl(position):
    # Distribution Focal Loss (DFL)
    import torch
    x = torch.tensor(position)
    n,c,h,w = x.shape
    p_num = 4
    mc = c//p_num
    y = x.reshape(n,p_num,mc,h,w)
    y = y.softmax(2)
    acc_metrix = torch.tensor(range(mc)).float().reshape(1,1,mc,1,1)
    y = (y*acc_metrix).sum(2)
    return y.numpy()


def box_process(position, anchors, args):
    grid_h, grid_w = position.shape[2:4]
    col, row = np.meshgrid(np.arange(0, grid_w), np.arange(0, grid_h))
    col = col.reshape(1, 1, grid_h, grid_w)
    row = row.reshape(1, 1, grid_h, grid_w)
    grid = np.concatenate((col, row), axis=1)
    stride = np.array([IMG_SIZE[1]//grid_h, IMG_SIZE[0]//grid_w]).reshape(1,2,1,1)

    if args.model in ['yolov5', 'yolov7', 'yolox']:
        # output format: xywh -> xyxy
        if args.model == 'yolox':
            box_xy = position[:,:2,:,:]
            box_wh = np.exp(position[:,2:4,:,:]) * stride
        elif args.model in ['yolov5', 'yolov7']:
            col = col.repeat(len(anchors), axis=0)
            row = row.repeat(len(anchors), axis=0)
            anchors = np.array(anchors)
            anchors = anchors.reshape(*anchors.shape, 1, 1)

            box_xy = position[:,:2,:,:]*2 - 0.5
            box_wh = pow(position[:,2:4,:,:]*2, 2) * anchors

        box_xy += grid
        box_xy *= stride
        box = np.concatenate((box_xy, box_wh), axis=1)

        # Convert [c_x, c_y, w, h] to [x1, y1, x2, y2]
        xyxy = np.copy(box)
        xyxy[:, 0, :, :] = box[:, 0, :, :] - box[:, 2, :, :]/ 2  # top left x
        xyxy[:, 1, :, :] = box[:, 1, :, :] - box[:, 3, :, :]/ 2  # top left y
        xyxy[:, 2, :, :] = box[:, 0, :, :] + box[:, 2, :, :]/ 2  # bottom right x
        xyxy[:, 3, :, :] = box[:, 1, :, :] + box[:, 3, :, :]/ 2  # bottom right y

    elif args.model == 'yolov6' and position.shape[1]==4:
        box_xy  = grid +0.5 -position[:,0:2,:,:]
        box_xy2 = grid +0.5 +position[:,2:4,:,:]
        xyxy = np.concatenate((box_xy*stride, box_xy2*stride), axis=1)

    elif args.model in ['yolov6', 'yolov8', 'ppyoloe_plus']:
        position = dfl(position)
        box_xy  = grid +0.5 -position[:,0:2,:,:]
        box_xy2 = grid +0.5 +position[:,2:4,:,:]
        xyxy = np.concatenate((box_xy*stride, box_xy2*stride), axis=1)

    return xyxy

def post_process(input_data, anchors, args):
    boxes, scores, classes_conf = [], [], []
    if args.model in ['yolov5', 'yolov7', 'yolox']:
        # 1*255*h*w -> 3*85*h*w
        input_data = [_in.reshape([len(anchors[0]),-1]+list(_in.shape[-2:])) for _in in input_data]
        for i in range(len(input_data)):
            boxes.append(box_process(input_data[i][:,:4,:,:], anchors[i], args))
            scores.append(input_data[i][:,4:5,:,:])
            classes_conf.append(input_data[i][:,5:,:,:])
    elif args.model in ['yolov6', 'yolov8', 'ppyoloe_plus']:
        defualt_branch=3
        pair_per_branch = len(input_data)//defualt_branch
        # Python 忽略 score_sum 输出
        for i in range(defualt_branch):
            boxes.append(box_process(input_data[pair_per_branch*i], None, args))
            classes_conf.append(input_data[pair_per_branch*i+1])
            scores.append(np.ones_like(input_data[pair_per_branch*i+1][:,:1,:,:], dtype=np.float32))

    def sp_flatten(_in):
        ch = _in.shape[1]
        _in = _in.transpose(0,2,3,1)
        return _in.reshape(-1, ch)

    boxes = [sp_flatten(_v) for _v in boxes]
    classes_conf = [sp_flatten(_v) for _v in classes_conf]
    scores = [sp_flatten(_v) for _v in scores]

    boxes = np.concatenate(boxes)
    classes_conf = np.concatenate(classes_conf)
    scores = np.concatenate(scores)

    # filter according to threshold
    boxes, classes, scores = filter_boxes(boxes, scores, classes_conf, args)

    # nms
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
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = [int(_b) for _b in box]
        print('class: {}, score: {}'.format(CLASSES[cl], score))
        print('box coordinate left,top,right,down: [{}, {}, {}, {}]'.format(top, left, right, bottom))

        cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6, (0, 0, 255), 2)

def setup_model(args):
    model_path = args.model_path
    if model_path.endswith('.pt') or model_path.endswith('.torchscript'):
        platform = 'pytorch'
        from common.framework_executor.pytorch_executor import Torch_model_container
        model = Torch_model_container(args.model_path)
    elif model_path.endswith('.rknn'):
        platform = 'rknn'
        from common.framework_executor.rknn_executor import RKNN_model_container 
        model = RKNN_model_container(args.model_path, args.target, args.device_id)
    elif model_path.endswith('onnx'):
        platform = 'onnx'
        from common.framework_executor.onnx_executor import ONNX_model_container
        model = ONNX_model_container(args.model_path)
    else:
        assert False, "{} is not rknn/pytorch/onnx model".format(model_path)
    print('Model-{} is {} model, starting val'.format(model_path, platform))
    return model, platform

def img_check(path):
    img_type = ['.jpg', '.jpeg', '.png', '.bmp']
    for _type in img_type:
        if path.endswith(_type) or path.endswith(_type.upper()):
            return True
    return False

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')
    # basic params
    parser.add_argument('--model_path', type=str, required= True, help='model path, could be .pt or .rknn file')
    parser.add_argument('--img_show', action='store_true', default=False, help='draw the result and show')
    parser.add_argument('--img_save', action='store_true', default=False, help='save the result')
    parser.add_argument('--target', type=str, default='rk1808', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')

    # data params
    parser.add_argument('--anno_json', type=str, default='../../../../../datasets/COCO/annotations/instances_val2017.json', help='coco annotation path')
    parser.add_argument('--img_folder', type=str, default='../../../../../datasets/COCO//val2017', help='img folder path')
    parser.add_argument('--coco_map_test', action='store_true', help='enable coco map test')

    # optional params
    parser.add_argument('--anchors', type=str, default='anchors_yolov5.txt', help='target to anchor file, only yolov5, yolov7 need this param')
    parser.add_argument('--std', type=int, default=255, help='std using for preprocess')
    parser.add_argument('--class_agnostic', type=bool, default=False, help='Default: False. [False: nms on all class. True: nms on each class]')
    parser.add_argument('--color', type=str, default='RGB', help='input img color')

    SUPPORT_MODEL = ['yolov5', 'yolov7', 'yolov6', 'yolox', 'yolov8', 'ppyoloe_plus']
    parser.add_argument('--model', type=str, default='yolov5', help='model type, support {}'.format(SUPPORT_MODEL))

    args = parser.parse_args()

    if args.model in EX_SUPPORT_MAP.keys():
        args.model = EX_SUPPORT_MAP[args.model]
    if args.model not in SUPPORT_MODEL:
        print('ERROR: {} model type is not support.'.format(args.model))
        exit()
    if args.coco_map_test:
        print(' \n Warning!!!!!!!!!!!!!!!!!Test coco,be careful that OBJ_THRESH would be set to 0.001 ,img_show and img_save would be disabled \n')
        OBJ_THRESH = 0.001
        args.img_show = False
        args.img_save = False

    # seting defualt hyperparam
    if args.model == 'yolox':
        args.anchors = None
        args.std = 1
        args.class_agnostic = True
        if args.model_path.endswith('.rknn'):
            args.color = 'RGB'
        else:
            args.color = 'BGR'
    elif args.model in ['yolov6','yolov8', 'ppyoloe_plus']:
        args.anchors = None

    # load anchor if needed
    if args.anchors is None or args.anchors =='None':
        print("None anchors file determine, free anchors mode")
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
    model, platform = setup_model(args)

    img_list = os.listdir(args.img_folder)
    co_helper = COCO_test_helper(enable_letter_box=True)

    # run test
    for i in range(len(img_list)):
        if not img_check(img_list[i]):
            continue
        print('finish {}/{}'.format(i+1, len(img_list)), end='\r')

        img_name = img_list[i]
        img_path = os.path.join(args.img_folder, img_name)
        if not os.path.exists(img_path):
            print("{} is not found", img_name)
            continue

        img_src = cv2.imread(img_path)
        if img_src is None:
            continue

        '''
        # using for test input dumped by C.demo
        img_src = np.fromfile('./input_b/demo_c_input_hwc_rgb.txt', dtype=np.uint8).reshape(640,640,3)
        img_src = cv2.cvtColor(img_src, cv2.COLOR_RGB2BGR)
        '''

        # Due to rga init with (0,0,0), we using pad_color (0,0,0) instead of (114, 114, 114)
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
        boxes, classes, scores = post_process(outputs, anchors, args)

        if args.img_show or args.img_save:
            print('\n\nIMG: {}'.format(img_name))
            img_p = img_src.copy()
            if boxes is not None:
                draw(img_p, co_helper.get_real_box(boxes), scores, classes)

            if args.img_save:
                if not os.path.exists('./yolo_output'):
                    os.mkdir('./yolo_output')
                cv2.imwrite(os.path.join('./yolo_output', img_name), img_p)
            
            if args.img_show:
                cv2.imshow("full post process result", img_p)
                cv2.waitKeyEx(0)

        # record maps
        if args.coco_map_test is True:
            if boxes is not None:
                for i in range(boxes.shape[0]):
                    co_helper.add_single_record(image_id = int(img_name.split('.')[0]),
                                                category_id = coco_id_list[int(classes[i])],
                                                bbox = boxes[i],
                                                score = round(scores[i], 5).astype(np.float)
                                                )

    # calculate maps
    if args.coco_map_test is True:
        pred_json = args.model_path.split('.')[-2]+ '_{}'.format(platform) +'.json'
        pred_json = pred_json.split('/')[-1]
        pred_json = os.path.join('./', pred_json)
        co_helper.export_to_json(pred_json)

        from utils.coco_utils import coco_eval_with_json
        coco_eval_with_json(args.anno_json, pred_json)


