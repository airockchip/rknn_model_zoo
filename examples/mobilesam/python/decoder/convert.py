import numpy as np
import sys
import cv2
from rknn.api import RKNN

DEFAULT_RKNN_PATH = '../../model/mobilesam_decoder.rknn'
DEFAULT_QUANT = False
NUM_POINTS = 2

dummy_inputs = {
    "image_embeddings": [1, 256, 28, 28],
    "point_coords": [1, NUM_POINTS, 2],
    "point_labels": [1, NUM_POINTS],
    "mask_input": [1, 1, 112, 112],
    "has_mask_input": [1],
    # "orig_im_size": [2],
}

outputs = [
    # 'masks',
    'iou_predictions',
    'low_res_masks',
]

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]))
        print("       platform choose from [rk3562, rk3576, rk3588]")
        print("       dtype choose from    [fp]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = DEFAULT_RKNN_PATH

    return model_path, platform, do_quant, output_path


if __name__ == '__main__':
    model_path, platform, do_quant, output_path = parse_arg()

    # Create RKNN object
    rknn = RKNN(verbose=True)

    # Pre-process config
    print('--> Config model')
    rknn.config(target_platform=platform,)
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=model_path,
                         inputs=[*dummy_inputs.keys()],
                         input_size_list=[*dummy_inputs.values()],
                         outputs=outputs
                         )
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=do_quant)
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