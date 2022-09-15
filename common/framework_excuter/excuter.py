import numpy as np
import os
import sys

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))

class Excuter:
    def __init__(self, framework_excute_info):
        # print('framework_excute_info', framework_excute_info)
        framework = framework_excute_info['model_framework']
        _info = framework_excute_info

        if framework == 'caffe':
            from caffe_excute import Caffe_model_container
            model_container = Caffe_model_container(
                 prototxt = _info['prototxt'],
                 caffemodel = _info['caffemodel'],
                 output_nodes = _info['output_nodes'],
                 mean_values = _info['mean_values'],
                 std_values = _info['std_values'],
            )

        elif framework == 'darknet':
            from darknet_excute import Darknet_model_container
            model_container = Darknet_model_container(
                 model_cfg = _info['model'],
                 model_weights = _info['weight'],
                 mean_values = _info['mean_values'],
                 std_values = _info['std_values'],
            )

        elif framework == 'mxnet':
            from mxnet_excute import Mxnet_model_container
            model_container = Mxnet_model_container(
                 symbol = _info['symbol'],
                 params = _info['params'],
                 input_nodes = None,
            )

        elif framework == 'onnx':
            from onnx_excute import ONNX_model_container
            model_container = ONNX_model_container(_info['model'])

        elif framework == 'pytorch':
            from pytorch_excute import Torch_model_container
            model_container = Torch_model_container(_info['model'])

        elif framework == 'keras':
            from keras_excute import Keras_model_container
            model_container = Keras_model_container(_info['model'])

        elif framework == 'tensorflow':
            from tensorflow_excute import Tensorflow_model_container
            model_container = Tensorflow_model_container(
                model_path = _info['model'], 
                inputs = _info['input_nodes'],
                outputs = _info['output_nodes'],
            )

        elif framework == 'tflite':
            from tflite_excute import Tflite_model_container
            model_container = Tflite_model_container(_info['model'])

        self.model_container = model_container

    def inference(self, inputs):
        return self.model_container.run(inputs)
    