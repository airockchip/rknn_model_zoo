from code import interact
import cv2
import os
import logging
import numpy as np
import re
import time
import glob
import signal

from threading import Thread
from pathlib import Path


IMG_FORMATS = ['bmp', 'jpg', 'jpeg', 'png', 'tif', 'tiff', 'dng', 'webp', 'mpo']  # acceptable image suffixes
VID_FORMATS = ['mov', 'avi', 'mp4', 'mpg', 'mpeg', 'm4v', 'wmv', 'mkv']  # acceptable video suffixes

def set_logging(name=None, verbose=True):
    # Sets level and returns logger
    rank = int(os.getenv('RANK', -1))  # rank in world for Multi-GPU trainings
    logging.basicConfig(format="%(message)s", level=logging.INFO if (verbose and rank in (-1, 0)) else logging.WARNING)
    return logging.getLogger(name)


LOGGER = set_logging(__name__)  # define globally (used in train.py, val.py, detect.py, etc.)


def clean_str(s):
    # Cleans a string by replacing special characters with underscore _
    return re.sub(pattern="[|@#!¡·$€%&()=?¿^*;:,¨´><+]", repl="_", string=s)



def letterbox(im, new_shape=(640, 640), color=(114, 114, 114), scaleup=True):
    # Resize and pad image while meeting stride-multiple constraints
    shape = im.shape[:2]  # current shape [height, width]
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)

    # Scale ratio (new / old)
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    if not scaleup:  # only scale down, do not scale up (for better val mAP)
        r = min(r, 1.0)

    # Compute padding
    ratio = r, r  # width, height ratios
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]  # wh padding

    dw /= 2  # divide padding into 2 sides
    dh /= 2

    im = cv2.UMat(im)
    if shape[::-1] != new_unpad:  # resize
        im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)  # add border
    return im, ratio, (dw, dh)


class LoadStreams:
    # YOLOv5 streamloader, i.e. `python detect.py --source 'rtsp://example.com/media.mp4'  # RTSP, RTMP, HTTP streams`
    def __init__(self, sources='streams.txt', img_size=640, save_dir="./detect_result/", interval_time=1):
        self.mode = 'stream'
        self.img_size = img_size
        self.interval_time = interval_time
        # save_dir
        save_dir = os.path.abspath(save_dir) + "/"
        if not os.path.exists(save_dir):
            os.mkdir(save_dir)        
        self.save_dir = save_dir

        if sources.endswith('.txt') and os.path.isfile(sources):
            sources = os.path.abspath(sources)
            with open(sources) as f:
                sources = [x.strip() for x in f.read().strip().splitlines() if len(x.strip())]
        else:
            sources = [sources]

        n = len(sources)
        self.imgs, self.fps, self.frames, self.threads, self.caps = [None] * n, [0] * n, [0] * n, [None] * n, [None] * n
        self.sources = [clean_str(x) for x in sources]  # clean source names for later
        for i, s in enumerate(sources):  # index, source
            st = f'{i + 1}/{n}: {s}... '
            s = eval(s) if s.isnumeric() else s  # i.e. s = '0' local webcam

            cap = cv2.VideoCapture(s, cv2.CAP_GSTREAMER)
            assert cap.isOpened(), f'{st}Failed to open {s}'
            self.caps[i] = cap
            w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            # fps:12.5
            self.fps[i] = max(cap.get(cv2.CAP_PROP_FPS) % 100, 0) or 30.0  # 30 FPS fallback
            # frame:inf
            self.frames[i] = max(int(cap.get(cv2.CAP_PROP_FRAME_COUNT)), 0) or float('inf')  # infinite stream fallback


            # _, self.imgs[i] = cv2.UMat(cap.read()[1])  # guarantee first frame
            _, self.imgs[i] = cap.read()
            self.threads[i] = Thread(target=self.update, args=([i, cap, s]), daemon=True)
            LOGGER.info(f"{st} Success ({self.frames[i]} frames {w}x{h} at {self.fps[i]:.2f} FPS)")
            self.threads[i].start()
        LOGGER.info('')  # newline

        # check for common shapes
        s = np.stack([cv2.UMat.get(letterbox(x, self.img_size)[0]).shape for x in self.imgs])
        # s = np.stack([letterbox(x, self.img_size)[0].shape for x in self.imgs])
        self.rect = np.unique(s, axis=0).shape[0] == 1  # rect inference if all shapes equal
        if not self.rect:
            LOGGER.warning('WARNING: Stream shapes differ. For optimal performance supply similarly-shaped streams.')

    def update(self, i, cap, stream):
        # Read stream `i` frames in daemon thread
        n, f, read = 0, self.frames[i], self.interval_time * self.fps[i] # frame number, frame array, inference every 'read' frame
        while cap.isOpened() and n < f:
            # _, self.imgs[index] = cap.read()
            n += 1
            cap.grab()
            if n % read == 0:
                success, im = cap.retrieve()
                if success:
                    # self.imgs[i] = cv2.UMat(im)
                    self.imgs[i] = im
                else:
                    LOGGER.warning('WARNING: Video stream unresponsive, please check your IP camera connection.')
                    # self.imgs[i] = cv2.UMat(np.zeros_like(self.imgs[i]))
                    self.imgs[i] = np.zeros_like(self.imgs[i])
                    cap.open(stream)  # re-open stream if signal was lost
            time.sleep(1/self.fps[i])
            # cap.set(cv2.CAP_PROP_POS_FRAMES, 0)

    def __iter__(self):
        self.count = -1
        return self

    def __next__(self):
        self.count += 1
        if not all(x.is_alive() for x in self.threads):
            print("one of threads exit")
            raise StopIteration

        # Letterbox
        img0 = self.imgs.copy()
        img = [letterbox(x, self.img_size)[0] for x in img0]

        # Stack
        # img = np.stack(img, 0)

        # Convert
        # img = img[..., ::-1].transpose((0, 3, 1, 2))  # BGR to RGB, BHWC to BCHW
        # img = np.ascontiguousarray(img)

        return self.sources, img, None, ''

    def __len__(self):
        return len(self.sources)  # 1E12 frames = 32 streams at 30 FPS for 30 years


class LoadImages:
    # YOLOv5 image/video dataloader, i.e. `python detect.py --source image.jpg/vid.mp4`
    def __init__(self, path, img_size=640, save_dir="./detect_result/"):
        p = str(Path(path).resolve())  # os-agnostic absolute path
        if '*' in p:
            files = sorted(glob.glob(p, recursive=True))  # glob
        elif os.path.isdir(p):
            files = sorted(glob.glob(os.path.join(p, '*.*')))  # dir
        elif os.path.isfile(p):
            files = [p]  # files
        else:
            raise Exception(f'ERROR: {p} does not exist')

        images = [x for x in files if x.split('.')[-1].lower() in IMG_FORMATS]
        videos = [x for x in files if x.split('.')[-1].lower() in VID_FORMATS]
        ni, nv = len(images), len(videos)

        self.img_size = img_size
        self.files = images + videos
        
        # save_dir
        save_dir = os.path.abspath(save_dir) + "/"
        if not os.path.exists(save_dir):
            os.mkdir(save_dir)        

        self.save_dir = save_dir
        self.save_video = None
        self.nf = ni + nv  # number of files
        self.video_flag = [False] * ni + [True] * nv
        self.mode = 'image'
        if any(videos):
            self.new_video(videos[0])  # new video
        else:
            self.cap = None
        assert self.nf > 0, f'No images or videos found in {p}. ' \
                            f'Supported formats are:\nimages: {IMG_FORMATS}\nvideos: {VID_FORMATS}'

    def __iter__(self):
        self.count = 0
        return self

    def __next__(self):
        if self.count == self.nf:
            raise StopIteration
        path = self.files[self.count]

        if self.video_flag[self.count]:
            # Read video
            self.mode = 'video'
            ret_val, img0 = self.cap.read()
            while not ret_val:
                self.count += 1
                self.cap.release()
                if self.count == self.nf:  # last video
                    raise StopIteration
                else:
                    path = self.files[self.count]
                    self.new_video(path)
                    ret_val, img0 = self.cap.read()


            time.sleep(1/self.fps)
            self.frame += 1
            s = f'video {self.count + 1}/{self.nf} ({self.frame}/{self.frames}) {path}: '

        else:
            # Read image
            self.count += 1
            img0 = cv2.imread(path)  # BGR
            assert img0 is not None, f'Image Not Found {path}'
            s = f'image {self.count}/{self.nf} {path}: '

        # Padded resize
        # img0 = cv2.UMat(img0)
        img = [letterbox(img0, self.img_size)[0]]

        # # Convert
        # img = img.transpose((2, 0, 1))[::-1]  # HWC to CHW, BGR to RGB
        # img = np.ascontiguousarray(img)

        return path, img, self.cap, s

    def new_video(self, path):
        self.frame = 0
        self.cap = cv2.VideoCapture(path, cv2.CAP_GSTREAMER)
        self.frames = int(self.cap.get(cv2.CAP_PROP_FRAME_COUNT))
        self.fps = max(self.cap.get(cv2.CAP_PROP_FPS) % 100, 0) or 30.0
        w, h = 0, 0
        if isinstance(self.img_size , int):
            w, h = self.img_size, self.img_size 
        else:
            w, h = self.img_size[0], self.img_size[1]
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        self.save_video = cv2.VideoWriter("{0}{1}{2}".format(self.save_dir, os.path.basename(path).split(".")[0], "_output.mp4"), fourcc, self.fps, (w, h))


    def __len__(self):
        return self.nf  # number of files


if __name__ == "__main__":
    # source = "/mnt/hgfs/virtualmachineshare/PythonProject/edgeml/huoyan1.mp4"
    # source = "/root/mgl/rknn_model_zoo/models/vision/object_detection/yolov5-pytorch/test_data/huoyan1.mp4"
    # source = "/mnt/hgfs/virtualmachineshare/rknn_model_zoo/datasets/fire/fire_00007.jpg"
    # dataset = LoadImages(source, img_size=640)

    # source = "rtsp://192.168.8.12:554/live/test"
    # source = "rtsp://admin:admin123@192.168.31.105:554/"
    # source = "/mnt/hgfs/virtualmachineshare/PythonProject/edgeml/rknn_model_zoo/models/vision/object_detection/yolov5-pytorch/RKNN_python_demo/utils/streams.txt"
    # dataset = LoadStreams(source, img_size=640)
    source = os.path.abspath('/root/mgl/rknn_model_zoo/models/vision/object_detection/yolov5-pytorch/RKNN_python_demo/utils/streams.txt')
    dataset = LoadStreams(sources=source)
    # dataset = LoadWebcam()

    # source = "rtsp://192.168.17.12:554/11"
    # dataset = LoadWebcam(source, img_size=640)
    # last = time.time()
    def siginalHanler(signum, frame):
        print('Get signal:', signum)
        if isinstance(dataset, LoadStreams):
            for cap in dataset.caps:
                if cap != None:
                    cap.release()
        else:
            if dataset.cap != None:
                dataset.cap.release()
            if dataset.save_video != None:
                dataset.save_video.release()
        exit()
    signal.signal(signal.SIGINT, siginalHanler)
    j = 0
    try: 
        for _, img, _, _ in dataset:
            for i, im in enumerate(img):
                if isinstance(dataset, LoadImages) and dataset.mode == "image":
                    cv2.imwrite(dataset.save_dir + "output.jpg", im)
                elif isinstance(dataset, LoadImages) and dataset.mode == "video":
                    # time.sleep(1)
                    print("yes")
                    dataset.save_video.write(im)
                else:
                    time.sleep(2)
                    # cv2.imwrite("{0}{1}{2}{3}{4}".format("output", j, "_", i, ".jpg"), im)
                    print("yes")
            j += 1
    except BaseException as e:
        if isinstance(e, KeyboardInterrupt):
            if isinstance(dataset, LoadStreams):
                for cap in dataset.caps:
                    if cap != None:
                        cap.release()
            else:
                if dataset.cap != None:
                    dataset.cap.release()
                if dataset.save_video != None:
                    dataset.save_video.release()
            exit()
        raise e

    if isinstance(dataset, LoadStreams):
        for cap in dataset.caps:
            if cap != None:
                cap.release()
    else:
        if dataset.cap != None:
            dataset.cap.release()
        if dataset.save_video != None:
            dataset.save_video.release()
    exit()    


