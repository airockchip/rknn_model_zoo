import numpy as np
import sys
import cv2
import os
from rknn.api import RKNN

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
DATASET = _sep.join(realpath[:len(realpath) - realpath[::-1].index("examples")]) + '/yolo_world/model/dataset.txt'


DEFAULT_RKNN_PATH = '../../model/yolo_world_v2s.rknn'
DEFAULT_QUANT = True
IMAGE_SIZE = [640, 640]
TEXT_INPUT_SIZE = 80 # Detect classes
TEXT_EMBEDS = 512

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]))
        print("       platform choose from [rk3562, rk3566, rk3568, rk3588, rk3576]")
        print("       dtype choose from    [fp, i8]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type in ['i8']:
            do_quant = True
        else:
            do_quant = False

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = DEFAULT_RKNN_PATH

    return model_path, platform, do_quant, output_path


if __name__ == '__main__':
    model_path, platform, do_quant, output_path = parse_arg()

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')
    rknn.config(target_platform=platform,
                mean_values=[[0, 0, 0]],
                std_values=[[255, 255, 255]],)
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=model_path,
                         inputs=['images', 'texts'],
                         input_size_list=[[1, 3, IMAGE_SIZE[0], IMAGE_SIZE[1]], [1, TEXT_INPUT_SIZE, TEXT_EMBEDS]])
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=do_quant, dataset=DATASET)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')

    # Release
    rknn.release()