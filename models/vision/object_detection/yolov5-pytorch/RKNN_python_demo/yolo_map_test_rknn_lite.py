import os
import cv2
import sys
import argparse
import time
import json
import signal

import numpy as np

from operator import truediv
from pathlib import Path
from utils.dataloaders import LoadImages, LoadStreams, IMG_FORMATS, VID_FORMATS
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


OBJ_THRESH = 0.25
NMS_THRESH = 0.65
IMG_SIZE = (640, 640)  # (width, height), such as (1280, 736)

CLASSES = ("fire", )

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
    anchors = [[10.0, 13.0], [16.0, 30.0], [33.0, 23.0], [30.0, 61.0], [62.0, 45.0], [59.0, 119.0], [116.0, 90.0], [156.0, 198.0], [373.0, 326.0]]

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
    parser.add_argument('--send_event', action='store_true', default=False, help='send fire event to unix domain socket')
    parser.add_argument('--uds_path', type=str, default='/tmp/uds_socket', help='unix domain socket path')
    parser.add_argument('--interval_time', type=int, default=1, help='detect interval time')
    # data params
    parser.add_argument('--anno_json', type=str, default='../../../../../datasets/COCO/annotations/instances_val2017.json', help='datasets annotation path')
    parser.add_argument('--source', type=str, default='./streams.txt', help='source path')
    parser.add_argument('--map_test', action='store_true', help='enable map test')
    parser.add_argument('--eval_perf', action="store_true", help='eval performance')
    parser.add_argument('--conf_thres', type=float, default=0.25, help='confidence threshold')
    parser.add_argument('--iou_thres', type=float, default=0.65, help='NMS IoU threshold')
    args = parser.parse_args()
    args.imgsz *= 2 if len(args.imgsz) == 1 else 1  # expand
    global OBJ_THRESH, NMS_THRESH
    OBJ_THRESH = args.conf_thres
    NMS_THRESH = args.iou_thres
    return args
      

def file_detect_process(args, dataset, boxes, scores, classes, client, i, img):
    has_fire = False
    if boxes is not None:
        print("detect")
        has_fire = True
        if args.img_save is True:
             # method 1 + uds socket
            img_p = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
            img = draw(img_p, boxes, scores, classes)
            if not dataset.save_video:
                cv2.imwrite("{0}{1}{2}{3}".format(dataset.save_dir, "output_", str(i), ".jpg"), img)   
            else:
                dataset.save_video.write(img)              
    else:               
        print("not detect")
        has_fire = False
        if dataset.save_video:
            dataset.save_video.write(img)  


    if args.send_event:
        check_fire = False
        if has_fire:
            check_fire = True
        # has fire
        ip = get_rtsp_ip(args.source)[i]
        data = {
            "CheckFire": check_fire,
            "DeviceIp": ip
        }
        msg = json.dumps(data)
        # interval 1 second to send 3 times for avoiding receiver dump which make it can't receive msg
        client.send(msg, 1, 3)  

def detect(args):

    is_file = Path(args.source).suffix[1:] in (IMG_FORMATS + VID_FORMATS)
    is_url = args.source.lower().startswith(('rtsp://', 'rtmp://', 'http://', 'https://'))
    webcam = args.source.isnumeric() or args.source.endswith('.txt') or (is_url and not is_file)

    # init model
    model_path = args.model_path
    if model_path.endswith('.pt') or model_path.endswith('.torchscript'):
        platform = 'pytorch'
        from common.framework_excuter.pytorch_excute import Torch_model_container
        model = Torch_model_container(args.model_path)
    elif model_path.endswith('.rknn'):
        platform = 'rknn'
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
        dataset = LoadStreams(args.source, img_size=args.imgsz, save_dir=args.save_dir, interval_time = args.interval_time)
    else:
        dataset = LoadImages(args.source, img_size=args.imgsz, save_dir=args.save_dir)

    # Run inference
    seen = 0
    dt = [0.0]
    num = len(dataset) if len(dataset) else 1
    has_detect_num_list = [0] * num
    has_not_detect_num_list = [0] * num
    max_detect_num_list = [3] * num

    has_send_stop_list = [True] * num
    output_seq_list = [0] * num

    client = None
    try:
        if args.send_event:
            # server_address = "/userdata/liug/stream/uds_socket/192.168.172.104:8080"
            # server_address = "/tmp/uds_socket"
            client = SocketClient(server_address=args.uds_path)
            client.connect_to_server()

        def siginalHanler(signum, frame):
            if isinstance(dataset, LoadStreams):
                for cap in dataset.caps:
                    if cap != None:
                        cap.release()
            else:
                if dataset.cap != None:
                    dataset.cap.release()
                if dataset.save_video != None:
                    dataset.save_video.release()
            if args.send_event:
                if not client:
                    client.close()
            model.release()
            exit()
        signal.signal(signal.SIGINT, siginalHanler)  
        
        for path, img_list, vid_cap, s in dataset:
            for i, img in enumerate(img_list):
                t1 = time.time()
                # Inference
                img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                
                input_data = cv2.UMat.get(img)
                # preprocee if not rknn model
                if platform in ['pytorch', 'onnx']:
                    input_data = input_data.transpose((2,0,1))
                    input_data = input_data.reshape(1,*input_data.shape).astype(np.float32)
                    input_data = input_data/255

                outputs = model.run([input_data])
                t2 = time.time()
                dt[0] = t2 - t1
                # proprocess result
                outputs = [output.reshape([3,-1]+list(output.shape[-2:])) for output in outputs]
                outputs = [np.transpose(output, (2,3,0,1)) for output in outputs]

                boxes, classes, scores = yolov5_post_process(outputs)
                if scores is not None:
                    seen += scores.size

                if webcam:
                    # detect 
                    if boxes is not None:
                        has_detect_num_list[i] += 1
                        has_not_detect_num_list[i] = 0
                        print('{0}{1}'.format('detect:', has_detect_num_list[i]))
                        if args.img_save is True:
                            if has_detect_num_list[i] >= max_detect_num_list[i]: 
                                has_detect_num_list[i] = 0             
                                print("detect")
                                # method 1 + uds socket
                                img_p = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
                                img = draw(img_p, boxes, scores, classes)
                                # print(img.shape)
                                print("{0}{1}{2}{3}{4}{5}".format(dataset.save_dir, "output_", str(i), "_", output_seq_list[i], ".jpg"))
                                cv2.imwrite("{0}{1}{2}{3}{4}{5}".format(dataset.save_dir, "output_", str(i), "_", output_seq_list[i], ".jpg"), img)
                                output_seq_list[i] += 1
                                if args.send_event:
                                    # has fire
                                    ip = get_rtsp_ip(args.source)[i]
                                    data = {
                                        "CheckFire": True,
                                        "DeviceIp": ip
                                    }
                                    msg = json.dumps(data)
                                    # interval 1 second to send 3 times for avoiding receiver dump which make it can't receive msg
                                    client.send(msg, 1, 3)
                                    has_send_stop_list[i] = False                        
                    else:
                        has_not_detect_num_list[i] += 1
                        has_detect_num_list[i] = 0
                        print('{0}{1}'.format('not_detect:', has_not_detect_num_list[i]))
                        if has_not_detect_num_list[i] >= max_detect_num_list[i]:
                            has_not_detect_num_list[i] = 0                                   
                            print("not detect")
                            if args.send_event:
                                if not has_send_stop_list[i]:
                                    # send uds not fire                                     
                                    ip = get_rtsp_ip(args.source)[i]
                                    data = {
                                        "CheckFire": False,
                                        "DeviceIp": ip
                                    }
                                    msg = json.dumps(data)
                                    # interval 1 second to send 3 times for avoiding receiver dump which make it can't receive msg
                                    client.send(msg, 1, 3)
                                    has_send_stop_list[i] = True                    
                else:
                    file_detect_process(args, dataset, boxes, scores, classes, client, i, img)
            if webcam:
                time.sleep(args.interval_time)

        t = tuple(x / seen * 1E3 for x in dt)  # speeds per image per detecede box unit:ms
        print("per image per detected box speend {0} ms".format(t[0]))
    except BaseException as e:     
        if isinstance(e, SystemExit):
            pass
        else:
            print("except:" + str(e))
        if isinstance(dataset, LoadStreams):
            for cap in dataset.caps:
                if cap != None:
                    cap.release()
        else:
            if dataset.cap != None:
                dataset.cap.release()
            if dataset.save_video != None:
                dataset.save_video.release()
        if args.send_event:
            if not client:
                client.close()
        model.release()
        raise e
        exit()   
    

    if isinstance(dataset, LoadStreams):
        for cap in dataset.caps:
            if cap != None:
                cap.release()
    else:
        if dataset.cap != None:
            dataset.cap.release()
        if dataset.save_video != None:
            dataset.save_video.release()
    if args.send_event:
        if not client:
            client.close()
    model.release()
    exit()        

if __name__ == '__main__':

    args = parse_opt()
    detect(args)





