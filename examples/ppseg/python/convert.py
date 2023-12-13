from rknn.api import RKNN
import numpy as np
import sys

DATASET_PATH = '../model/dataset.txt'
MEAN = [[0.485*255, 0.456*255, 0.406*255]]
STD = [[0.229*255, 0.224*255, 0.225*255]]

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 {} pt_model_path [rk3566|rk3588|rk3562] [i8/fp (optional)] [output_path (optional)]".format(sys.argv[0]));
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type == 'i8':
            do_quant = True
            print("ERROR: i8 quantization is not supported yet, ppseg-i8 drop accuracy!")
            exit(1)
        else:
            do_quant = False
    else:
        do_quant = True

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = '../model/pp_liteseg.rknn'

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=MEAN, std_values=STD, target_platform=platform)
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=model_path)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=do_quant, dataset=DATASET_PATH)
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
