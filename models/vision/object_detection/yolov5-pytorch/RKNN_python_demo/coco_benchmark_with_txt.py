from utils.coco_utils import COCO_test_helper,coco_eval_with_json
import numpy as np
import sys

CLASSES = ['person', 'bicycle', 'car', 'motorcycle', 'airplane', 'bus', 'train', 'truck', 'boat', 'traffic light',
        'fire hydrant', 'stop sign', 'parking meter', 'bench', 'bird', 'cat', 'dog', 'horse', 'sheep', 'cow',
        'elephant', 'bear', 'zebra', 'giraffe', 'backpack', 'umbrella', 'handbag', 'tie', 'suitcase', 'frisbee',
        'skis', 'snowboard', 'sports ball', 'kite', 'baseball bat', 'baseball glove', 'skateboard', 'surfboard',
        'tennis racket', 'bottle', 'wine glass', 'cup', 'fork', 'knife', 'spoon', 'bowl', 'banana', 'apple',
        'sandwich', 'orange', 'broccoli', 'carrot', 'hot dog', 'pizza', 'donut', 'cake', 'chair', 'couch',
        'potted plant', 'bed', 'dining table', 'toilet', 'tv', 'laptop', 'mouse', 'remote', 'keyboard', 'cell phone',
        'microwave', 'oven', 'toaster', 'sink', 'refrigerator', 'book', 'clock', 'vase', 'scissors', 'teddy bear',
        'hair drier', 'toothbrush']

coco_id_list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 31, 32, 33, 34,
         35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
         64, 65, 67, 70, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 89, 90]


def main(path):
    with open(path,'r') as f:
        results = f.readlines()

    co_help = COCO_test_helper()

    for i in range(len(results)):
        img_name, class_name, class_index, score, box = results[i].split(',')
        img_id = int(img_name.split('/')[-1].split('.')[0])
        category_id = coco_id_list[CLASSES.index(class_name)]
        score = np.float(score)
        box = box.rstrip('\n')[1:-1]
        bbox = [int(_v) for _v in box.split(' ')]

        co_help.add_single_record(img_id, category_id, bbox, score)
        print("parse {}/{}".format(i+1, len(results)))

    out_path = 'tmp_annotation_result.json'
    co_help.export_to_json(out_path)
    coco_eval_with_json('../../../../../datasets/COCO/annotations/instances_val2017.json', out_path)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Using as\npython coco_benchmark_with_txt.py ./result_record.txt')
        exit()
    path = sys.argv[1]
    main(path)