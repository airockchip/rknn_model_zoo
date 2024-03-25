import os
import sys
import urllib
import time
import traceback
import numpy as np
import cv2
from rknn.api import RKNN
from scipy.special import softmax

DATASET_PATH = '../../../datasets/imagenet/ILSVRC2012_img_val_samples/dataset_20.txt'
DEFAULT_RKNN_PATH = '../model/resnet50-v2-7.rknn'
DEFAULT_ONNX_PATH = '../model/resnet50-v2-7.onnx'
CLASS_LABEL_PATH = '../model/synset.txt'
DEFAULT_QUANT = True

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
    if not os.path.exists(DEFAULT_ONNX_PATH):
        print('--> Download {}'.format(DEFAULT_ONNX_PATH))
        url = 'https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7.onnx'
        download_file = DEFAULT_ONNX_PATH
        try:
            start_time = time.time()
            urllib.request.urlretrieve(url, download_file, show_progress)
        except:
            print('Download {} failed.'.format(download_file))
            print(traceback.format_exc())
            exit(-1)
        print('done')

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} [onnx_model_path] [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]))
        print("       platform choose from [rk3562,rk3566,rk3568,rk3588,rk1808,rv1109,rv1126]")
        print("       dtype choose from [i8, fp] for [rk3562,rk3566,rk3568,rk3588]")
        print("       dtype choose from [u8, fp] for [rk1808,rv1109,rv1126]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['u8', 'i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type in ['i8', 'u8']:
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

    if model_path == DEFAULT_ONNX_PATH:
        # Download model if not exist (from https://github.com/onnx/models/tree/main/vision/classification/resnet)
        check_and_download_origin_model()

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[[255*0.485, 255*0.456, 255*0.406]], std_values=[[255*0.229, 255*0.224, 255*0.225]], target_platform=platform)
    print('done')

    # Load model
    print('--> Loading model')
    if platform.lower() in RKNPU1_TARGET:
        ret = rknn.load_onnx(model=model_path, inputs=['data'], input_size_list=[[3, 224, 224]])
    else:
        ret = rknn.load_onnx(model=model_path, inputs=['data'], input_size_list=[[1, 3, 224, 224]])

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

    # Set inputs
    img = cv2.imread('../model/dog_224x224.jpg')
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (224, 224))
    img = np.expand_dims(img, 0)

    # Init runtime environment
    print('--> Init runtime environment')
    if platform.lower() in RKNPU1_TARGET:
        # For RKNPU1, the simulator has beed disabled since version 1.7.5
        ret = rknn.init_runtime(target=platform)
    else:
        ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
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

    # Release
    rknn.release()
