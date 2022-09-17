import os
import cv2
import sys
import argparse
import time
import json

import numpy as np

from operator import truediv
from pathlib import Path
from utils.dataloaders import LoadImages, LoadStreams, IMG_FORMATS, VID_FORMATS, LoadWebcam
from utils.uds import SocketClient, get_rtsp_ip


# add path
realpath = os.path.abspath(__file__)
print(realpath)
_sep = os.path.sep
realpath = realpath.split(_sep)
# '/mnt/hgfs/virtualmachineshare/rknn_model_zoo/models/vision/object_detection/yolov5-pytorch/RKNN_python_demo'
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
# '/mnt/hgfs/virtualmachineshare/rknn_model_zoo'
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('rknn_model_zoo')+1]))

# todo 
OBJ_THRESH = 0.1
NMS_THRESH = 0.65
IMG_SIZE = (640, 640)  # (width, height), such as (1280, 736)

CLASSES = ("fire",)

# todo
# class_id_list = [1]



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

def process(input, mask, anchors):

    anchors = [anchors[i] for i in mask]
    grid_h, grid_w = map(int, input.shape[0:2])

    box_confidence = sigmoid(input[..., 4])
    box_confidence = np.expand_dims(box_confidence, axis=-1)

    box_class_probs = sigmoid(input[..., 5:])

    box_xy = sigmoid(input[..., :2])*2 - 0.5

    col = np.tile(np.arange(0, grid_w), grid_h).reshape(-1, grid_w)
    row = np.tile(np.arange(0, grid_h).reshape(-1, 1), grid_w)
    col = col.reshape(grid_h, grid_w, 1, 1).repeat(3, axis=-2)
    row = row.reshape(grid_h, grid_w, 1, 1).repeat(3, axis=-2)
    grid = np.concatenate((col, row), axis=-1)
    box_xy += grid
    box_xy *= (int(IMG_SIZE[1]/grid_h), int(IMG_SIZE[0]/grid_w))

    box_wh = pow(sigmoid(input[..., 2:4])*2, 2)
    box_wh = box_wh * anchors

    box = np.concatenate((box_xy, box_wh), axis=-1)

    return box, box_confidence, box_class_probs

def filter_boxes(boxes, box_confidences, box_class_probs):
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
    box_scores = box_confidences * box_class_probs
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


def yolov5_post_process(input_data):
    masks = [[0, 1, 2], [3, 4, 5], [6, 7, 8]]
    anchors = [[10.0, 13.0], [16.0, 30.0], [33.0, 23.0], [30.0, 61.0], 
    [62.0, 45.0], [59.0, 119.0], [116.0, 90.0], [156.0, 198.0], [373.0, 326.0]]

    boxes, classes, scores = [], [], []
    for input,mask in zip(input_data, masks):
        b, c, s = process(input, mask, anchors)
        b, c, s = filter_boxes(b, c, s)
        boxes.append(b)
        classes.append(c)
        scores.append(s)

    boxes = np.concatenate(boxes)
    boxes = xywh2xyxy(boxes)
    classes = np.concatenate(classes)
    scores = np.concatenate(scores)
  

    nboxes, nclasses, nscores = [], [], []
    for c in set(classes):
        inds = np.where(classes == c)
        b = boxes[inds]
        c = classes[inds]
        s = scores[inds]

        keep = nms_boxes(b, s)

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
    return image


def parse_opt():

    parser = argparse.ArgumentParser(description='Process some integers.')
    # basic params required= True, 
    parser.add_argument('--model_path', type=str, help='model path, could be .onnx or .rknn file')
    parser.add_argument('--img_save', action='store_true', default=False, help='draw the result and save')
    parser.add_argument('--save_dir', type=str, default='./detect_result/', help='path for detected result to save')
    parser.add_argument('--imgsz', '--img', '--img_size', nargs='+', type=int, default=[640, 640], help='inference size h,w')

    # data params
    parser.add_argument('--anno_json', type=str, default='../../../../../datasets/COCO/annotations/instances_val2017.json', help='datasets annotation path')
    parser.add_argument('--source', type=str, default='../../../../../datasets/COCO//val2017', help='source path')
    parser.add_argument('--map_test', action='store_true', help='enable map test')
    parser.add_argument('--eval_perf', action="store_true", help='eval performance')
    args = parser.parse_args()
    args.imgsz *= 2 if len(args.imgsz) == 1 else 1  # expand
    # todo
    # print_args(vars(opt))
    return args


def detect(args):

    is_file = Path(args.source).suffix[1:] in (IMG_FORMATS + VID_FORMATS)
    is_url = args.source.lower().startswith(('rtsp://', 'rtmp://', 'http://', 'https://'))
    webcam = args.source.isnumeric() or (is_url and not is_file)

    # save_dir
    args.save_dir = os.path.abspath(args.save_dir) + "/"
    if not os.path.exists(args.save_dir):
        os.mkdir(args.save_dir)

    print(args.save_dir)
    # todo
    # pt model.stride

    # init model
    model_path = args.model_path
    if model_path.endswith('.pt') or model_path.endswith('.torchscript'):
        platform = 'pytorch'
        from common.framework_excuter.pytorch_excute import Torch_model_container
        model = Torch_model_container(args.model_path)
    elif model_path.endswith('.rknn'):
        platform = 'rknn'
        # todo rknn_execute
        from common.framework_excuter.rknn_lite_excute import RKNN_model_container 
        model = RKNN_model_container(args.model_path)
    elif model_path.endswith('onnx'):
        platform = 'onnx'
        from common.framework_excuter.onnx_excute import ONNX_model_container
        model = ONNX_model_container(args.model_path)
    else:
        assert False, "{} is not rknn/pytorch/onnx model".format(model_path)
    print('Model-{} is {} model, starting val'.format(model_path, platform))


    # Dataloader
    if webcam:
        # LoadWebcam
        dataset = LoadWebcam(args.source, img_size=args.imgsz, save_dir=args.save_dir)
        # LoadStreams    
        # dataset = LoadStreams(args.source, img_size=args.imgsz, save_dir=args.save_dir)
    else:
        dataset = LoadImages(args.source, img_size=args.imgsz, save_dir=args.save_dir)

    # Run inference
    seen = 0
    dt = [0.0, 0.0]
    has_detect_num = 0
    # many dectect to ensure 
    max_detect_num = 3
    last_detect_time = time.time()
    last_has_fire_time = 0
    interval_detect_time = 1
    has_fire = False
    has_not_fire_time_num = 10
    has_send_stop = True
    output_seq = 0
    try:
        # server_address = "/userdata/liug/stream/uds_socket/192.168.172.104:8080"
        server_address = "/tmp/uds_socket"
        client = SocketClient(server_address=server_address)
        client.connect_to_server()       
        for path, img, im0s, vid_cap, s in dataset:

            if time.time() - last_detect_time < interval_detect_time:
                continue
            last_detect_time = time.time()
            if isinstance(dataset, LoadStreams):
                img = img[0]
            t1 = time.time()
            # todo
            # im to half or float 
            # im = im.half() if model.fp16 else im.float()  # uint8 to fp16/32
            # im /= 255  # 0 - 255 to 0.0 - 1.0
            t2 = time.time()
            dt[0] += t2 - t1
            # Inference
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

            # preprocee if not rknn model
            if platform in ['pytorch', 'onnx']:
                input_data = img.transpose((2,0,1))
                input_data = input_data.reshape(1,*input_data.shape).astype(np.float32)
                input_data = input_data/255
            else:
                input_data = img


            # todo output
            outputs = model.run([input_data])
            t3 = time.time()
            dt[1] += t3 - t2
            # proprocess result
            outputs = [output.reshape([3,-1]+list(output.shape[-2:])) for output in outputs]
            outputs = [np.transpose(output, (2,3,0,1)) for output in outputs]

            boxes, classes, scores = yolov5_post_process(outputs)
            if scores is not None:
                seen += scores.size

            # detect 
            if boxes is not None:
                print("detect")
                has_fire = True
                last_has_fire_time = time.time()
                has_detect_num += 1
                if has_detect_num >= max_detect_num:
                    # method 1 + uds socket
                    if args.img_save is True:
                        img_p = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
                        img = draw(img_p, boxes, scores, classes)
                        print(img.shape)
                        print("{0}{1}{2}{3}".format(args.save_dir, "output", output_seq, ".jpg"))
                        cv2.imwrite("{0}{1}{2}{3}".format(args.save_dir, "output", output_seq, ".jpg"), img)
                        output_seq += 1
                    has_detect_num = 0
                    # has fire
                    ip = get_rtsp_ip(args.source)
                    data = {
                        "CheckFire": True,
                        "DeviceIp": ip
                    }
                    msg = json.dumps(data)

                    # 间隔3秒发送三次，防止接收端应用挂了，接收不到
                    client.send(msg, 1, 2)
                    has_send_stop = False

                # method 2
                # img_p = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
                # img = draw(img_p, boxes, scores, classes)
                # write the BGR frame
                # if isinstance(dataset, LoadImages) and dataset.mode == "image":
                #     cv2.imwrite(args.save_dir + "output.jpg", img)
                # else:
                #     dataset.save_video.write(img)
            else:
                has_fire = False

            if not has_fire and time.time() - last_has_fire_time > has_not_fire_time_num and not has_send_stop:
                # send uds not fire                                     
                ip = get_rtsp_ip(args.source)
                data = {
                    "CheckFire": False,
                    "DeviceIp": ip
                }
                msg = json.dumps(data)
                # 间隔3秒发送三次，防止接收端应用挂了，接收不到
                client.send(msg, 1, 2)
                has_send_stop = True

        t = tuple(x / seen * 1E3 for x in dt)  # speeds per image per detecede box unit:ms
        print("per image per detected box speend {0} ms".format(t[1]))
    except BaseException as e:
        if isinstance(e, KeyboardInterrupt):
            if isinstance(dataset, LoadStreams):
                for cap in dataset.caps:
                    cap.release()
            else:
                if dataset.cap != None:
                    dataset.cap.release()
            dataset.save_video.release()
            print("exit")
        else:
            print("except:" + str(e))
        if not client:
            client.close()


    # print(model.eval_perf())
    # print(model.eval_memory())
    # release model
    model.release()



if __name__ == '__main__':

    args = parse_opt()
    # args.model_path = "../RKNN_model_convert/model.rknn"
    # args.source = "/mnt/hgfs/virtualmachineshare/rknn_model_zoo/datasets/fire/fire_00007.jpg"
    # args.imgsz = 640
    # args.img_save = True
    detect(args)





