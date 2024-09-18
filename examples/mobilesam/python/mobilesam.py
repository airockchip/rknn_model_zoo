import numpy as np
import cv2
import torch.nn.functional as F
import torch
from rknn.api import RKNN
import argparse


IMG_SIZE = 448
MASK_THRESHOLD = 0.0


def get_preprocess_shape(oldh, oldw):
    scale = IMG_SIZE * 1.0 / max(oldh, oldw)
    newh, neww = oldh * scale, oldw * scale
    newh, neww = int(newh + 0.5), int(neww + 0.5)

    return (newh, neww)


def img_preprocess(img_path):
    img = cv2.imread(img_path)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    oldh, oldw = img.shape[:2]

    newh, neww = get_preprocess_shape(oldh, oldw)

    padh, padw = IMG_SIZE - newh, IMG_SIZE - neww

    img = cv2.resize(img, (neww, newh), interpolation=cv2.INTER_LINEAR)
    img = cv2.copyMakeBorder(img, 0, padh, 0, padw, cv2.BORDER_CONSTANT, value=(0, 0, 0))  # add border
    img = np.array([img]).astype(np.float32)

    return img

def coords_preprocess(coords, ori_shape):
    oldh, oldw = ori_shape
    newh, neww = get_preprocess_shape(oldh, oldw)

    coords[..., 0] = coords[..., 0] * (neww / oldw)
    coords[..., 1] = coords[..., 1] * (newh / oldh)

    return coords

def postprocess(masks, input_shape, ori_shape):
    masks = F.interpolate(torch.from_numpy(masks), (IMG_SIZE, IMG_SIZE), mode='bilinear')

    masks = masks[..., : input_shape[0], : input_shape[1]]
    masks = F.interpolate(masks, ori_shape, mode='bilinear')

    return masks.numpy()

def draw(images, masks, coords, labels, color=(144, 144, 30)):
    alpha = 0.5
    h, w = masks.shape[-2:]
    color = np.array(color)
    mask_image = masks.reshape(h, w, 1) * color.reshape(1, 1, -1).astype(np.uint8)
    images = np.where(mask_image != 0, cv2.add(alpha * images, (1 - alpha) * mask_image), images)

    top, left, right, bottom = None, None, None, None
    for coords, labels in zip(args.point_coords, args.point_labels):
        if labels == 0:
            cv2.circle(images, tuple(coords), 12, color=(0, 0, 255), thickness=2)
        elif labels == 1:
            cv2.circle(images, tuple(coords), 12, color=(0, 255, 0), thickness=2)
        elif labels in [2, 3]:
            if labels == 2:
                top, left = coords
            elif labels == 3:
                right, bottom = coords
            if top and left and right and bottom:
                cv2.rectangle(images, (top, left), (right, bottom), (0, 255, 0), 2)

    cv2.imwrite('result.jpg', images)
    print('result save to result.jpg')


class MobileSAM():
    def __init__(self, args):
        self.encoder = args.encoder
        self.decoder = args.decoder
        self.target = args.target
        self.img = args.img
        self.point_coords = np.array(args.point_coords).astype(np.float32)
        self.point_labels = np.array(args.point_labels).astype(np.float32)
        self.mask_input = args.mask_input

    def encoder_run(self):
        rknn = RKNN(verbose=False)
        rknn.load_rknn(self.encoder)
        rknn.init_runtime(target=self.target)
        img = img_preprocess(self.img)
        outputs = rknn.inference(inputs=[img])[0]

        rknn.release()

        return outputs

    def decoder_run(self, img_embeds):
        point_coords = coords_preprocess(self.point_coords[None, :, :], cv2.imread(self.img).shape[:2])
        point_labels = self.point_labels[None, :]
        # has_mask_input: An indicator for the mask input. 1 indicates a mask input, 0 indicates no mask input.
        if args.mask_input:
            mask_input = np.load(args.mask_input).astype(np.float32)
            has_mask_input = np.ones(1, dtype=np.float32)
        else:
            mask_input = np.zeros((1, 1, 112, 112), dtype=np.float32)
            has_mask_input = np.zeros(1, dtype=np.float32)

        rknn = RKNN(verbose=False)
        rknn.load_rknn(self.decoder)
        rknn.init_runtime(target=self.target)
        outputs = rknn.inference(inputs=[img_embeds, point_coords, point_labels, mask_input, has_mask_input], data_format='NCHW')

        rknn.release()

        return outputs[0], outputs[1]

    def run(self):
        img_embeds = self.encoder_run()
        iou_predictions, low_res_masks= self.decoder_run(img_embeds)

        return iou_predictions, low_res_masks


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='mobilesam rknn model test')
    parser.add_argument('--encoder',
                        type=str,
                        help="mobilesam encoder rknn model path",
                        required=True)
    parser.add_argument('--decoder',
                        type=str,
                        help="mobilesam decoder rknn model path",
                        required=True)
    parser.add_argument('--target',
                        type=str,
                        help="target platform",
                        required=True)
    parser.add_argument('--img',
                        type=str,
                        help="input image",
                        default='../model/picture.jpg')
    parser.add_argument('--point_coords',
                        type=list,
                        help="point inputs and box inputs.Boxes are encoded using two points, one for the top-left corner and one for the bottom-right corner, \
                        such as points and boxes: [[400, 400], [0, 0]] and [[190, 70], [460, 280]]",
                        default=[[190, 70], [460, 280]])
    parser.add_argument('--point_labels',
                        type=list,
                        help="0 is a negative input point, 1 is a positive input point, 2 is a top-left box corner, 3 is a bottom-right box corner, and -1 is a padding point, \
                        if there is no box input, a single padding point with label -1 and point_coords (0.0, 0.0) should be concatenated.",
                        default=[2, 3])
    parser.add_argument('--mask_input',
                        type=str,
                        help=" Mask input path to the model, .npy format. Default is zeros.",
                        default=None)

    args = parser.parse_args()
    mobilesam = MobileSAM(args)
    scores, low_res_masks = mobilesam.run()

    # postprocess
    images = cv2.imread(args.img)
    ori_shape = images.shape[:2]
    input_shape = get_preprocess_shape(ori_shape[0], ori_shape[1])
    masks = postprocess(low_res_masks, input_shape, ori_shape)

    masks = masks > MASK_THRESHOLD
    masks = masks[:, np.argmax(scores), :, :]

    draw(images, masks, args.point_coords, args.point_labels, color=(144, 144, 30))
