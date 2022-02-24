import cv2
import numpy as np
import copy

SUPPORT_COLOR_TYPE = ['RGB','BGR','GRAY']

class Image_preprocessor():
    def __init__(self, img_path, color_type) -> None:
        self.img_path = img_path
        self.color_type = color_type
        self.load_img(self.img_path, self.color_type)

        self.preprocess_record = {}

    def load_img(self, img_path, color_type):
        # loading img
        assert color_type in SUPPORT_COLOR_TYPE, "now only support color type as follow: {}".format(SUPPORT_COLOR_TYPE)
        if color_type == 'GRAY':
            self.img = cv2.imread(img_path, 'GRAY')
        else:
            self.img = cv2.imread(img_path)

        if color_type == 'RGB':
            self.img = cv2.cvtColor(self.img, cv2.COLOR_BGR2RGB)
        elif color_type == 'BGR':
            pass

    def normalize(self, mean_values, std_values):
        #! WARNING, after normalize, the img is float format, not uint8 format anymore.
        _img = copy.deepcopy(self.img).astype(np.float32)
        for i in range(len(mean_values)):
            _img[:,:,i] = (_img[:,:,i] - mean_values[i])/std_values[i]
        self.img = _img

    def resize(self, target_size):
        _img = copy.deepcopy(self.img)
        if isinstance(target_size, list):
            target_size = tuple(target_size)
        _img = cv2.resize(self.img, (target_size[1],target_size[0])) # model got hwc info, but cv2 need wh
        self.img = _img

    # TODO
    def letter_box(self, target_size):
        pass

    def get_input(self, framework, rknn_passthrough=False):
        def rknn_type():
            if rknn_passthrough==True:
                output = self.img.transpose(2, 0, 1)
            else:
                output = self.img
            return output 
        
        def pytorch_type():
            output = self.img.transpose(2, 0, 1)
            output = output.reshape(1,*output.shape)
            return output
        
        def tf_type():
            output = self.img.reshape(1,*self.img.shape)
            return output

        def caffe_type():
            output = self.img
            return output

        framework_type_dict = {
            'caffe': caffe_type,
            'darknet': caffe_type,
            
            'mxnet': pytorch_type,
            'onnx': pytorch_type,
            'pytorch': pytorch_type,

            'keras': tf_type,
            'tensorflow': tf_type,
            'tflite': tf_type,

            'rknn': rknn_type,
        }

        type_function = framework_type_dict[framework]
        return type_function()