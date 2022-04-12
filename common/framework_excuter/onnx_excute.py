import numpy as np
import onnxruntime as rt


class ONNX_model_container:
    def __init__(self, model_path) -> None:
        self.model_path = model_path
        self.sess = rt.InferenceSession(model_path, providers=['TensorrtExecutionProvider', 'CUDAExecutionProvider', 'CPUExecutionProvider'])

    def run(self, input_datas):
        if len(input_datas) < len(self.sess.get_inputs()):
            assert False,'inputs_datas number not match onnx model{} input'.format(self.model_path)
        elif len(input_datas) > len(self.sess.get_inputs()):
            print('WARNING: input datas number large than onnx input node')

        input_dict = {}
        for i in range(len(self.sess.get_inputs())):
            input_dict[self.sess.get_inputs()[i].name] = input_datas[i]

        output_list = []
        for i in range(len(self.sess.get_outputs())):
            output_list.append(self.sess.get_outputs()[i].name)

        #forward model
        res = self.sess.run(output_list, input_dict)
        return res