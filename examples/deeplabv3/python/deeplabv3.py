'''
Author: Chao Li 
Date: 2023-10-23 09:19:52
LastEditTime: 2024-01-29 14:41:42
Editors: Chao Li 
Description: Convert the deeplabv3 model trained by TensorFlow into RKNN model, and then use the RKNN model for inference.
'''
import numpy as np
from matplotlib import pyplot as plt
import cv2
from matplotlib import gridspec
import torch
import torch.nn.functional as F
from rknn.api import RKNN
import get_dataset_colormap
import argparse

TEST_IMG_PATH='../model/test_image.jpg'
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


def post_process(output):
    output = np.transpose(output, (0, 3, 1, 2))
    output = F.interpolate(torch.tensor(output), torch.Size(
        [OUT_SIZE, OUT_SIZE]), mode='bilinear', align_corners=False)
    output = np.transpose(output.numpy(), (0, 2, 3, 1))
    seg_img = np.argmax(output, axis=-1)
    seg_img = np.reshape(seg_img, (OUT_SIZE, OUT_SIZE)).astype(np.uint8)

    return seg_img

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='deeplabv3 Python Demo', add_help=True)
    # basic params
    parser.add_argument('--model_path', type=str, required=True,
                        help='model path, could be .rknn file')
    parser.add_argument('--target', type=str,
                        default='rk3566', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str,
                        default=None, help='device id')
    args = parser.parse_args()

    # Create RKNN object
    rknn = RKNN()

    # Load RKNN model
    ret = rknn.load_rknn(args.model_path)
    if ret != 0:
        print('Load RKNN model \"{}\" failed!'.format(args.model_path))
        exit(ret)
    print('done')

    # Set inputs
    img = cv2.imread(TEST_IMG_PATH)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (513, 513))

    # init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime(target=args.target, device_id=args.device_id)
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    # Inference
    print('--> Running model')
    outputs = rknn.inference(inputs=[img])
    print('--> done')

    seg_map = post_process(outputs[0])

    # plot img
    vis_segmentation(img, seg_map)

    # save result
    seg_img = get_dataset_colormap.label_to_color_image(
        seg_map, get_dataset_colormap.get_pascal_name()).astype(np.uint8)
    overlay = img*0.5 + seg_img*0.5
    overlay = overlay.astype(np.uint8)
    overlay = cv2.cvtColor(overlay, cv2.COLOR_RGB2BGR)
    cv2.imwrite('output.png', overlay)

    rknn.release()
