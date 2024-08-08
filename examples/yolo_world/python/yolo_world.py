import numpy as np
import cv2
from rknn.api import RKNN
import argparse
import itertools
from transformers import AutoTokenizer


OBJ_THRESH = 0.25
NMS_THRESH = 0.45

IMG_SIZE = [640, 640]
SEQUENCE_LEN = 20
PAD_VALUE = 49407

CLASSES = ("person", "bicycle", "car","motorbike ","aeroplane ","bus ","train","truck ","boat","traffic light",
           "fire hydrant","stop sign ","parking meter","bench","bird","cat","dog ","horse ","sheep","cow","elephant",
           "bear","zebra ","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite",
           "baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife ",
           "spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza ","donut","cake","chair","sofa",
           "pottedplant","bed","diningtable","toilet ","tvmonitor","laptop	","mouse	","remote ","keyboard ","cell phone","microwave ",
           "oven ","toaster","sink","refrigerator ","book","clock","vase","scissors ","teddy bear ","hair drier", "toothbrush ")

def text_tokenizer(text, model_name):
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    text = list(itertools.chain(*text))
    text = tokenizer(text=text, return_tensors='pt', padding=True)

    return np.array(text['input_ids'])

def letter_box(img, new_shape, pad_color=(0,0,0)):
    # Resize and pad image while meeting stride-multiple constraints
    shape = img.shape[:2]  # current shape [height, width]
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
        img = cv2.resize(img, new_unpad, interpolation=cv2.INTER_LINEAR)
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    img = cv2.copyMakeBorder(img, top, bottom, left, right, cv2.BORDER_CONSTANT, value=pad_color)  # add border

    return img, ratio, (dw, dh)

def img_preprocess(img_path):
    img = cv2.imread(img_path)
    img, _, _ = letter_box(img.copy(), new_shape=[IMG_SIZE[1], IMG_SIZE[0]], pad_color=(0, 0, 0))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = np.array([img]).astype(np.float32)

    return img

def filter_boxes(boxes, box_confidences, box_class_probs):
    """Filter boxes with object threshold.
    """
    box_confidences = box_confidences.reshape(-1)

    class_max_score = np.max(box_class_probs, axis=-1)
    classes = np.argmax(box_class_probs, axis=-1)

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

def box_process(position):
    grid_h, grid_w = position.shape[2:4]
    col, row = np.meshgrid(np.arange(0, grid_w), np.arange(0, grid_h))
    col = col.reshape(1, 1, grid_h, grid_w)
    row = row.reshape(1, 1, grid_h, grid_w)
    grid = np.concatenate((col, row), axis=1)
    stride = np.array([IMG_SIZE[1]//grid_h, IMG_SIZE[0]//grid_w]).reshape(1,2,1,1)

    box_xy  = grid +0.5 -position[:,0:2,:,:]
    box_xy2 = grid +0.5 +position[:,2:4,:,:]
    xyxy = np.concatenate((box_xy*stride, box_xy2*stride), axis=1)

    return xyxy

def postprocess(input_data):
    boxes, scores, classes_conf = [], [], []
    defualt_branch=3
    pair_per_branch = len(input_data)//defualt_branch
    # Python 忽略 score_sum 输出
    for i in range(defualt_branch):
        boxes.append(box_process(input_data[pair_per_branch*i+1]))
        classes_conf.append(input_data[pair_per_branch*i])
        scores.append(np.ones_like(input_data[pair_per_branch*i][:,:1,:,:], dtype=np.float32))

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
    boxes, classes, scores = filter_boxes(boxes, scores, classes_conf)

    # nms
    nboxes, nclasses, nscores = [], [], []
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
    print("{:^12} {:^12}  {}".format('class', 'score', 'xmin, ymin, xmax, ymax'))
    print('-' * 50)
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = box
        top = int(top)
        left = int(left)
        right = int(right)
        bottom = int(bottom)

        cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6, (0, 0, 255), 2)

        print("{:^12} {:^12.3f} [{:>4}, {:>4}, {:>4}, {:>4}]".format(CLASSES[cl], score, top, left, right, bottom))

class YOLOWORLD():
    def __init__(self, args):
        self.text_model = args.text_model
        self.yolo_world = args.yolo_world
        self.target = args.target
        self.img = args.img
        self.text = args.text

    def clip_text_run(self):
        input_ids = text_tokenizer(self.text, "openai/clip-vit-base-patch32")
        text_num, seq_len = input_ids.shape
        if seq_len >= SEQUENCE_LEN:
            input_data = input_ids[:, :SEQUENCE_LEN]
        else:
            input_data = np.zeros((text_num, SEQUENCE_LEN)).astype(np.float32)
            input_data[:, :seq_len] = input_ids
            input_data[:, seq_len:] = PAD_VALUE

        rknn = RKNN(verbose=False)
        rknn.load_rknn(self.text_model)
        rknn.init_runtime(target=self.target)
        outputs = []
        for i in range(text_num):
            outputs.append(rknn.inference(inputs=[input_data[i:i+1, :]])[0])

        return np.concatenate(outputs, axis=0)

    def yolo_world_run(self, text_input):

        rknn = RKNN(verbose=False)
        rknn.load_rknn(self.yolo_world)
        rknn.init_runtime(target=self.target)
        img = img_preprocess(self.img)
        outputs = rknn.inference(inputs=[img, text_input])

        return outputs

    def run(self):
        text_outputs = self.clip_text_run()

        yolo_outputs = self.yolo_world_run(text_outputs)

        return yolo_outputs


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='rknn model test')
    parser.add_argument('--text_model', type=str, help="clip_text model path", default='../model/clip_text.rknn')
    parser.add_argument('--yolo_world', type=str, help="yolo_world model path", default='../model/yolo_world_v2s.rknn')
    parser.add_argument('--target', type=str, help="target platform", required=True)
    parser.add_argument('--img', type=str, help="input image", default='../model/bus.jpg')
    parser.add_argument('--text', type=list, help="input text", default=[CLASSES])

    args = parser.parse_args()
    yoloworld = YOLOWORLD(args)
    outputs = yoloworld.run()

    # postprocess
    boxes, classes, scores = postprocess(outputs)

    img = cv2.imread(args.img)
    if boxes is not None:
        draw(img, boxes, scores, classes)
        cv2.imwrite('result.jpg', img)
        print('Save results to result.jpg!')

