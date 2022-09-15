import onnx
import onnxruntime

class onnx_runner:
    def __init__(self, model_path) -> None:
        self.sess = onnxruntime.InferenceSession(model_path)
        
        self.inputs_name_list = [_object.name for _object in self.sess.get_inputs()]
        self.outputs_name_list = [_object.name for _object in self.sess.get_outputs()]

        self.inputs_number = len(self.inputs_name_list)
        self.outputs_number = len(self.outputs_name_list) 
        
    def run(self, inputs):
        if isinstance(inputs, list) or isinstance(inputs, tuple):
            assert len(inputs) == self.inputs_number, "inputs number not match"
        else:
            assert self.inputs_number == 1, "inputs number not match"
            inputs = [inputs]

        for i in range(len(inputs)):
            inputs_dict = {self.inputs_name_list[i]: inputs[i]}

        result = self.sess.run(self.outputs_name_list, inputs_dict)
        return result

