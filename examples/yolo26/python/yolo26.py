import os
import cv2
import sys
import argparse

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('rknn_model_zoo')+1]))

from py_utils.coco_utils import COCO_test_helper
import numpy as np

import onnx
import onnxruntime

OBJ_THRESH = 0.25

IMG_SIZE = (640, 640)  # (width, height), such as (1280, 736)

# YOLO26 truncated output nodes
YOLO26_OUTPUTS = ['/model.23/Sigmoid_output_0', '/model.23/Mul_2_output_0']

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


def add_intermediate_outputs(model_path, output_names):
    """Add intermediate nodes as model outputs for truncation (ONNX only)"""
    model = onnx.load(model_path)
    
    # Check existing outputs
    existing_outputs = [obj.name for obj in model.graph.output]
    
    # Collect intermediate nodes info
    value_info_map = {}
    for node in model.graph.node:
        for output in node.output:
            if output in output_names and output not in existing_outputs:
                value_info_map[output] = onnx.helper.make_empty_tensor_value_info(output)
    
    # Add to model outputs
    for name in output_names:
        if name in value_info_map:
            model.graph.output.append(value_info_map[name])
    
    return model


def post_process(outputs, conf_threshold=OBJ_THRESH, topk=300):
    """
    YOLO26 one2one post-processing (No NMS needed)
    
    Args:
        outputs: list of model outputs [scores, boxes] or [boxes, scores]
        conf_threshold: confidence threshold
        topk: maximum number of detections
    
    Returns:
        boxes: (M, 4) - filtered boxes (x1, y1, x2, y2)
        classes: (M,) - class indices  
        scores: (M,) - confidence scores
    """
    # Determine which output is which based on shape
    # scores: (1, 80, 8400), boxes: (1, 4, 8400)
    if outputs[0].shape[1] == 4:
        boxes, scores = outputs[0], outputs[1]
    else:
        boxes, scores = outputs[1], outputs[0]
    
    if boxes.ndim == 3:
        boxes = boxes[0]
    if scores.ndim == 3:
        scores = scores[0]
        
    # (4, N) -> (N, 4)
    boxes = boxes.T
    # (num_classes, N) -> (N, num_classes)
    scores = scores.T
    
    # Get max class score and class id
    max_scores = scores.max(axis=1)
    class_ids = scores.argmax(axis=1)
    
    # Filter by threshold
    mask = max_scores > conf_threshold
    boxes = boxes[mask]
    max_scores = max_scores[mask]
    class_ids = class_ids[mask]
    
    if len(max_scores) == 0:
        return None, None, None
        
    # Sort and take TopK
    if len(max_scores) > topk:
        indices = np.argsort(max_scores)[::-1][:topk]
        boxes = boxes[indices]
        max_scores = max_scores[indices]
        class_ids = class_ids[indices]
    
    return boxes, class_ids, max_scores


def draw(image, boxes, scores, classes):
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = [int(_b) for _b in box]
        print("%s @ (%d %d %d %d) %.3f" % (CLASSES[cl], top, left, right, bottom, score))
        cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)


class ONNX_model_container:
    """ONNX model container with truncation support for YOLO26"""
    def __init__(self, model_path):
        print(f"Loading ONNX model with truncated outputs: {YOLO26_OUTPUTS}")
        model_proto = add_intermediate_outputs(model_path, YOLO26_OUTPUTS)
        self.session = onnxruntime.InferenceSession(
            model_proto.SerializeToString(), 
            providers=['CPUExecutionProvider']
        )
        self.input_name = self.session.get_inputs()[0].name
    
    def run(self, input_datas):
        return self.session.run(YOLO26_OUTPUTS, {self.input_name: input_datas[0]})
    
    def release(self):
        del self.session
        self.session = None


def setup_model(args):
    model_path = args.model_path
    if model_path.endswith('.rknn'):
        platform = 'rknn'
        from py_utils.rknn_executor import RKNN_model_container 
        model = RKNN_model_container(args.model_path, args.target, args.device_id)
    elif model_path.endswith('onnx'):
        platform = 'onnx'
        model = ONNX_model_container(args.model_path)
    else:
        assert False, "{} is not rknn/onnx model".format(model_path)
    print('Model-{} is {} model, starting val'.format(model_path, platform))
    return model, platform


def img_check(path):
    img_type = ['.jpg', '.jpeg', '.png', '.bmp']
    for _type in img_type:
        if path.endswith(_type) or path.endswith(_type.upper()):
            return True
    return False


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='YOLO26 inference demo')
    # basic params
    parser.add_argument('--model_path', type=str, required=True, help='model path, could be .onnx or .rknn file')
    parser.add_argument('--target', type=str, default='rk3566', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')
    
    parser.add_argument('--img_show', action='store_true', default=False, help='draw the result and show')
    parser.add_argument('--img_save', action='store_true', default=False, help='save the result')

    # data params
    parser.add_argument('--anno_json', type=str, default='../../../datasets/COCO/annotations/instances_val2017.json', help='coco annotation path')
    # coco val folder: '../../../datasets/COCO//val2017'
    parser.add_argument('--img_folder', type=str, default='../model', help='img folder path')
    parser.add_argument('--coco_map_test', action='store_true', help='enable coco map test')

    args = parser.parse_args()

    # init model
    model, platform = setup_model(args)

    file_list = sorted(os.listdir(args.img_folder))
    img_list = []
    for path in file_list:
        if img_check(path):
            img_list.append(path)
    co_helper = COCO_test_helper(enable_letter_box=True)

    # run test
    for i in range(len(img_list)):
        print('infer {}/{}'.format(i+1, len(img_list)), end='\r')

        img_name = img_list[i]
        img_path = os.path.join(args.img_folder, img_name)
        if not os.path.exists(img_path):
            print("{} is not found", img_name)
            continue

        img_src = cv2.imread(img_path)
        if img_src is None:
            continue

        # Due to rga init with (0,0,0), we using pad_color (0,0,0) instead of (114, 114, 114)
        pad_color = (0,0,0)
        img = co_helper.letter_box(im= img_src.copy(), new_shape=(IMG_SIZE[1], IMG_SIZE[0]), pad_color=(0,0,0))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

        # preprocess if not rknn model
        if platform in ['onnx']:
            input_data = img.transpose((2,0,1))
            input_data = input_data.reshape(1,*input_data.shape).astype(np.float32)
            input_data = input_data/255.
        else:
            input_data = img

        outputs = model.run([input_data])
        boxes, classes, scores = post_process(outputs)

        if args.img_show or args.img_save:
            print('\n\nIMG: {}'.format(img_name))
            img_p = img_src.copy()
            if boxes is not None:
                draw(img_p, co_helper.get_real_box(boxes), scores, classes)

            if args.img_save:
                if not os.path.exists('./result'):
                    os.mkdir('./result')
                result_path = os.path.join('./result', img_name)
                cv2.imwrite(result_path, img_p)
                print('Detection result save to {}'.format(result_path))
                        
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
                                                score = round(scores[i], 5).item()
                                                )

    # calculate maps
    if args.coco_map_test is True:
        pred_json = args.model_path.split('.')[-2]+ '_{}'.format(platform) +'.json'
        pred_json = pred_json.split('/')[-1]
        pred_json = os.path.join('./', pred_json)
        co_helper.export_to_json(pred_json)

        from py_utils.coco_utils import coco_eval_with_json
        coco_eval_with_json(args.anno_json, pred_json)

    # release
    model.release()

