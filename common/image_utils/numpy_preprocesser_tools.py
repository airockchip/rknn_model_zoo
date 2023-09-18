import numpy as np
import copy

class numpy_preprocessor():
    def __init__(self, in_path) -> None:
        self.in_path = in_path
        self.load_npy(self.in_path)

        self.preprocess_record = {}

    def load_npy(self, npy_path):
        # loading img
        self.data = np.load(npy_path)

    def normalize(self, mean_values, std_values):
        if len(self.data.shape) == 3:
            _data = copy.deepcopy(self.data)
            for i in range(len(mean_values)):
                _data[:,:,i] = (_data[:,:,i] - mean_values[i])/std_values[i]
        else:
            print('noly supprt 3D numpy array normalize')
            return

    def to_float(self):
        self.data = self.data.astype(np.float32)

    def check_and_reshape(self, _shape):
        pass

    def get_input(self, framework, rknn_passthrough=False):
        def rknn_type():
            output = self.data
            if output.dtype != np.float32:
                print('force data from {} to float32'.format(output.dtype))
                output = output.astype(np.float32)
            if len(output.shape) == 4:
                output = output.transpose(0, 2, 3, 1)   # nchw -> nhwc
            return output
        
        def pytorch_type():
            return self.data
        
        def tf_type():
            output = self.data.reshape(1,*self.data.shape)
            return output

        def caffe_type():
            return NotImplemented

        framework_type_dict = {
            'caffe': pytorch_type,
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