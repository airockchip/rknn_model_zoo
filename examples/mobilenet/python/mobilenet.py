import argparse
import os
import sys
import urllib
import urllib.request
import time
import traceback
import numpy as np
import cv2
from rknn.api import RKNN
from scipy.special import softmax

DATASET_PATH = '../../../datasets/imagenet/ILSVRC2012_img_val_samples/dataset_20.txt'
MODEL_DIR = '../model/'
MODEL_PATH = MODEL_DIR + 'mobilenetv2-12.onnx'
OUT_RKNN_PATH = MODEL_DIR + 'mobilenet_v2.rknn'
CLASS_LABEL_PATH = MODEL_DIR + 'synset.txt'

RKNPU1_TARGET = ['rk1808', 'rv1109', 'rv1126']


def readable_speed(speed):
    speed_bytes = float(speed)
    speed_kbytes = speed_bytes / 1024
    if speed_kbytes > 1024:
        speed_mbytes = speed_kbytes / 1024
        if speed_mbytes > 1024:
            speed_gbytes = speed_mbytes / 1024
            return "{:.2f} GB/s".format(speed_gbytes)
        else:
            return "{:.2f} MB/s".format(speed_mbytes)
    else:
        return "{:.2f} KB/s".format(speed_kbytes)


def show_progress(blocknum, blocksize, totalsize):
    speed = (blocknum * blocksize) / (time.time() - start_time)
    speed_str = " Speed: {}".format(readable_speed(speed))
    recv_size = blocknum * blocksize

    f = sys.stdout
    progress = (recv_size / totalsize)
    progress_str = "{:.2f}%".format(progress * 100)
    n = round(progress * 50)
    s = ('#' * n).ljust(50, '-')
    f.write(progress_str.ljust(8, ' ') + '[' + s + ']' + speed_str)
    f.flush()
    f.write('\r\n')


def check_and_download_origin_model():
    global start_time
    if not os.path.exists(MODEL_PATH):
        print('--> Download {}'.format(MODEL_PATH))
        url = 'https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/MobileNet/mobilenetv2-12.onnx'
        download_file = MODEL_PATH
        try:
            start_time = time.time()
            urllib.request.urlretrieve(url, download_file, show_progress)
        except:
            print('Download {} failed.'.format(download_file))
            print(traceback.format_exc())
            exit(-1)
        print('done')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='MobileNet Python Demo', add_help=True)
    parser.add_argument('--target', type=str,
                        default='rk3566', help='RKNPU target platform')
    parser.add_argument('--npu_device_test', action='store_true',
                        default=False, help='Connected npu device run')
    parser.add_argument('--accuracy_analysis', action='store_true',
                        default=False, help='Accuracy analysis')
    parser.add_argument('--eval_perf', action='store_true',
                        default=False, help='Time consuming evaluation')
    parser.add_argument('--eval_memory', action='store_true',
                        default=False, help='Memory evaluation')
    parser.add_argument('--model', type=str,
                        default=MODEL_PATH, help='onnx model path')
    parser.add_argument('--output_path', type=str,
                        default=OUT_RKNN_PATH, help='output rknn model path')
    parser.add_argument('--dtype', type=str, default='i8',
                        help='dtype of model, i8/fp32 for RKNPU2, u8/fp32 for RKNPU1')
    args = parser.parse_args()

    # Download model if not exist (from https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/MobileNet/mobilenetv2-12.onnx)
    check_and_download_origin_model()

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[[255*0.485, 255*0.456, 255*0.406]], std_values=[[
                255*0.229, 255*0.224, 255*0.225]], target_platform=args.target)
    print('done')

    # Load model
    print('--> Loading model')
    if args.target in RKNPU1_TARGET:
        ret = rknn.load_onnx(model=args.model, inputs=['input'], input_size_list=[[3,224,224]])
    else:
        ret = rknn.load_onnx(model=args.model, inputs=['input'], input_size_list=[[1,3,224,224]])

    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    do_quant = True if (args.dtype == 'i8' or args.dtype == 'u8') else False
    ret = rknn.build(do_quantization=do_quant, dataset=DATASET_PATH)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn(args.output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')

    # Set inputs
    img = cv2.imread('../model/bell.jpg')
    img = cv2.resize(img, (224, 224))
    img = np.expand_dims(img, 0)

    # Init runtime environment
    print('--> Init runtime environment')
    if args.npu_device_test or args.target in RKNPU1_TARGET:
        # For RKNPU1, the simulator has beed disabled since version 1.7.5
        ret = rknn.init_runtime(target=args.target)
    elif args.eval_perf or args.eval_memory:
        ret = rknn.init_runtime(
            target=args.target, perf_debug=True, eval_mem=True)
    else:
        if args.target in RKNPU1_TARGET:
            print('The target {} does not support simulator.'.format(args.target))
            print('Please set `--npu_device_test` to init runtime with real target.')
            exit(-1)
        ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')

    # Eval Perf
    if args.eval_perf:
        print('--> Eval Perf')
        rknn.eval_perf()
        print('done')

    # Eval Memory
    if args.eval_memory:
        print('--> Eval Memory')
        rknn.eval_memory()
        print('done')

    # Inference
    print('--> Running model')
    outputs = rknn.inference(inputs=[img])

    # Post Process
    print('--> PostProcess')
    with open(CLASS_LABEL_PATH, 'r') as f:
        labels = [l.rstrip() for l in f]

    scores = softmax(outputs[0])
    # print the top-5 inferences class
    scores = np.squeeze(scores)
    a = np.argsort(scores)[::-1]
    print('-----TOP 5-----')
    for i in a[0:5]:
        print('[%d] score=%.2f class="%s"' % (i, scores[i], labels[i]))
    print('done')

    # Accuracy analysis
    if args.accuracy_analysis:
        print('--> Accuracy analysis')
        if args.npu_device_test:
            ret = rknn.accuracy_analysis(
                inputs=['../model/bell.jpg'], target=args.target)
        else:
            ret = rknn.accuracy_analysis(inputs=['../model/bell.jpg'])
        if ret != 0:
            print('Accuracy analysis failed!')
            exit(ret)
        print('done')

    # Release
    rknn.release()
