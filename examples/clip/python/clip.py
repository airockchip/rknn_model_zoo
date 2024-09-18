import numpy as np
import cv2
from rknn.api import RKNN
import argparse
import itertools
from transformers import AutoTokenizer

IMAGE_SIZE = [224, 224]
SEQUENCE_LEN = 20
PAD_VALUE = 49407
CROP_SIZE = 224

def text_tokenizer(text, model_name):
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    text = list(itertools.chain(*text))
    text = tokenizer(text=text, return_tensors='pt', padding=True)

    return np.array(text['input_ids'])


def img_preprocess(img_path):
    img = cv2.imread(img_path)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    h, w, _ = img.shape
    if h < CROP_SIZE:
        padh = (CROP_SIZE - h) // 2
        img = np.pad(img, ((padh, CROP_SIZE - h - padh), (0, 0), (0, 0)), mode='constant').astype(np.uint8)
    if w < CROP_SIZE:
        padw = (CROP_SIZE - w) // 2
        img = np.pad(img, ((0, 0), (padw, CROP_SIZE - w - padw), (0, 0)), mode='constant').astype(np.uint8)
    if h > CROP_SIZE and w > CROP_SIZE:
        start_x = (w - CROP_SIZE) // 2
        start_y = (h - CROP_SIZE) // 2
        img = img[start_y:start_y+CROP_SIZE, start_x:start_x+CROP_SIZE]
    img = cv2.resize(img, (IMAGE_SIZE[0], IMAGE_SIZE[1]),)
    img = np.expand_dims(img, 0)

    return img

class CLIP():
    def __init__(self, args):
        self.text_model = args.text_model
        self.img_model = args.img_model
        self.target = args.target
        self.img = args.img
        self.text = args.text

    def clip_text_run(self):
        input_ids = text_tokenizer(self.text, "openai/clip-vit-base-patch32")
        text_num, seq_len = input_ids.shape
        if seq_len >= SEQUENCE_LEN:
            input_data = input_ids[:, :SEQUENCE_LEN]
        else:
            input_data = np.zeros((text_num, SEQUENCE_LEN)).astype(np.float32)
            input_data[:, :seq_len] = input_ids
            input_data[:, seq_len:] = PAD_VALUE

        rknn = RKNN()
        rknn.load_rknn(self.text_model)
        rknn.init_runtime(target=self.target)
        outputs = []
        for i in range(text_num):
            outputs.append(rknn.inference(inputs=[input_data[i:i+1, :]])[0])

        rknn.release()

        return np.concatenate(outputs, axis=0)

    def clip_img_run(self):
        rknn = RKNN()
        rknn.load_rknn(self.img_model)
        rknn.init_runtime(target=self.target)
        img = img_preprocess(self.img)
        outputs = rknn.inference(inputs=[img])

        rknn.release()

        return outputs[0]

    def run(self):
        text_outp = self.clip_text_run()
        img_outp = self.clip_img_run()

        res = np.matmul(text_outp, img_outp.reshape(512, 1))
        res = np.multiply(res, np.exp(4.605170249938965))
        res = np.exp(res)/np.sum(np.exp(res)) # softmax

        return res


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='rknn model test')
    parser.add_argument('--text_model', type=str, help="clip-text model path", default='../model/clip_text.rknn')
    parser.add_argument('--img_model', type=str, help="clip-images model path", default='../model/clip_images.rknn')
    parser.add_argument('--target', '-t', type=str, help="target platform", required=True)
    parser.add_argument('--img', type=str, help="input image", default='../model/dog_224x224.jpg')
    parser.add_argument('--text', type=list, help="input text", default=[['a photo of dog', 'a photo of cat']])

    args = parser.parse_args()
    clip = CLIP(args)
    outputs = clip.run()

    img_index = np.argmax(outputs) / len(args.text[0])
    text_index = np.argmax(outputs) % len(args.text[0])
    score = outputs.max()

    print("--> rknn clip demo result:")
    print("images: {:s}".format(args.img))
    print("text  : {:s}".format(args.text[0][text_index]))
    print("score : {:.3f}".format(score))