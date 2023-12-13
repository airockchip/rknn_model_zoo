'''
Author: Chao Li 
Date: 2023-10-23 09:19:52
LastEditTime: 2023-11-29 15:47:23
Editors: Chao Li 
Description: Convert the deeplabv3 model trained by TensorFlow into RKNN model, and then use the RKNN model for inference.
'''
import numpy as np
from matplotlib import pyplot as plt
import cv2

import sys

from matplotlib import gridspec

from rknn.api import RKNN

import get_dataset_colormap

TEST_IMG_PATH='../model/test_image.jpg'
DATASET_PATH='../model/dataset.txt'
DEFAULT_RKNN_PATH = '../model/deeplab-v3-plus-mobilenet-v2.rknn'
DEFAULT_QUANT = True

OUT_SIZE = 513

LABEL_NAMES = np.asarray([
    'background', 'aeroplane', 'bicycle', 'bird', 'boat', 'bottle',
    'bus', 'car', 'cat', 'chair', 'cow', 'diningtable', 'dog',
    'horse', 'motorbike', 'person', 'pottedplant', 'sheep', 'sofa',
    'train', 'tv'
])

FULL_LABEL_MAP = np.arange(len(LABEL_NAMES)).reshape(len(LABEL_NAMES), 1)
FULL_COLOR_MAP = get_dataset_colormap.label_to_color_image(FULL_LABEL_MAP)

def vis_segmentation(image, seg_map):
    plt.figure(figsize=(15, 5))
    grid_spec = gridspec.GridSpec(1, 4, width_ratios=[6, 6, 6, 1])

    plt.subplot(grid_spec[0])
    plt.imshow(image)
    plt.axis('off')
    plt.title('input image')

    plt.subplot(grid_spec[1])
    seg_image = get_dataset_colormap.label_to_color_image(
        seg_map, get_dataset_colormap.get_pascal_name()).astype(np.uint8)
    plt.imshow(seg_image)
    plt.axis('off')
    plt.title('segmentation map')

    plt.subplot(grid_spec[2])
    plt.imshow(image)
    plt.imshow(seg_image, alpha=0.7)
    plt.axis('off')
    plt.title('segmentation overlay')

    unique_labels = np.unique(seg_map)

    ax = plt.subplot(grid_spec[3])
    plt.imshow(FULL_COLOR_MAP[unique_labels].astype(np.uint8), interpolation='nearest')
    ax.yaxis.tick_right()
    plt.yticks(range(len(unique_labels)), LABEL_NAMES[unique_labels])
    plt.xticks([], [])
    ax.tick_params(width=0)

    plt.show()


def post_process(outputs):
    seg_img = np.argmax(outputs, axis=-1)
    seg_h = seg_img.shape[2]
    seg_w = seg_img.shape[1]
    seg_img = np.reshape(seg_img, (seg_w, seg_h)).astype(np.uint8)
    seg_img = cv2.resize(seg_img, (OUT_SIZE, OUT_SIZE))

    return seg_img

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} pb_model_path [platform] [dtype(optional)] [output_rknn_path(optional)] [plot/save(optional)]".format(sys.argv[0]));
        print("       platform choose from [rk3562,rk3566,rk3568,rk3588]")
        print("       dtype choose from    [i8, fp]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type == 'i8':
            do_quant = True
        else:
            do_quant = False

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = DEFAULT_RKNN_PATH

    if len(sys.argv) > 5:
        plot_control = sys.argv[5]
        assert plot_control in ['plot', 'save']
    else:
        plot_control = 'plot'

    return model_path, platform, do_quant, output_path, plot_control

if __name__ == '__main__':
    model_path, platform, do_quant, output_path, plot_control = parse_arg()

    # Create RKNN object
    rknn = RKNN()

    rknn.config(mean_values=[127.5, 127.5, 127.5], std_values=[127.5, 127.5, 127.5], quant_img_RGB2BGR=False,target_platform=platform)

    # Load model
    print('--> Loading model')

    rknn.load_tensorflow(model_path, 
                        inputs=['sub_7'],
                        outputs=['logits/semantic/BiasAdd'],
                        input_size_list=[[1,513,513,3]])
    print('done')

    # Build model
    print('--> Building model')
    rknn.build(do_quantization=do_quant , dataset=DATASET_PATH)
    print('done')

    # Set inputs
    img = cv2.imread(TEST_IMG_PATH)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (513, 513))

    # init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    # Inference
    print('--> Running model')
    outputs = rknn.inference(inputs=[img])
    print('--> done')

    seg_map = post_process(outputs[0])

    if plot_control == 'plot':
        vis_segmentation(img, seg_map)
    elif plot_control == 'save':
        seg_img = get_dataset_colormap.label_to_color_image(
            seg_map, get_dataset_colormap.get_pascal_name()).astype(np.uint8)
        overlay = img*0.5 + seg_img*0.5
        overlay = overlay.astype(np.uint8)
        overlay = cv2.cvtColor(overlay, cv2.COLOR_RGB2BGR)
        cv2.imwrite('output.png', overlay)

    rknn.export_rknn(output_path)

    rknn.release()
