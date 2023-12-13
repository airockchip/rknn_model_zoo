import os
import numpy as np
import onnxruntime as rt

type_map = {
    'tensor(int32)' : np.int32,
    'tensor(int64)' : np.int64,
    'tensor(float32)' : np.float32,
    'tensor(float64)' : np.float64,
    'tensor(float)' : np.float32,
}
if getattr(np, 'bool', False):
    type_map['tensor(bool)'] = np.bool
else:
    type_map['tensor(bool)'] = bool

def ignore_dim_with_zero(_shape, _shape_target):
    _shape = list(_shape)
    _shape_target = list(_shape_target)
    for i in range(_shape.count(1)):
        _shape.remove(1)
    for j in range(_shape_target.count(1)):
        _shape_target.remove(1)
    if _shape == _shape_target:
        return True
    else:
        return False


class ONNX_model_container_py:
    def __init__(self, model_path) -> None:
        # sess_options=
        sp_options = rt.SessionOptions()
        sp_options.log_severity_level = 3
        # [1 for info, 2 for warning, 3 for error, 4 for fatal] 
        self.sess = rt.InferenceSession(model_path, sess_options=sp_options, providers=['CPUExecutionProvider'])
        self.model_path = model_path

    def run(self, input_datas):
        if len(input_datas) < len(self.sess.get_inputs()):
            assert False,'inputs_datas number not match onnx model{} input'.format(self.model_path)
        elif len(input_datas) > len(self.sess.get_inputs()):
            print('WARNING: input datas number large than onnx input node')

        input_dict = {}
        for i, _input in enumerate(self.sess.get_inputs()):
            # convert type
            if _input.type in type_map and \
                type_map[_input.type] != input_datas[i].dtype:
                print('WARNING: force data-{} from {} to {}'.format(i, input_datas[i].dtype, type_map[_input.type]))
                input_datas[i] = input_datas[i].astype(type_map[_input.type])
            
            # reshape if need
            if _input.shape != list(input_datas[i].shape):
                if ignore_dim_with_zero(input_datas[i].shape,_input.shape):
                    input_datas[i] = input_datas[i].reshape(_input.shape)
                    print("WARNING: reshape inputdata-{}: from {} to {}".format(i, input_datas[i].shape, _input.shape))
                else:
                    assert False, 'input shape{} not match real data shape{}'.format(_input.shape, input_datas[i].shape)
            input_dict[_input.name] = input_datas[i]

        output_list = []
        for i in range(len(self.sess.get_outputs())):
            output_list.append(self.sess.get_outputs()[i].name)

        #forward model
        res = self.sess.run(output_list, input_dict)
        return res


class ONNX_model_container_cpp:
    def __init__(self, model_path) -> None:
        pass

    def run(self, input_datas):
        pass


def ONNX_model_container(model_path, backend='py'):
    if backend == 'py':
        return ONNX_model_container_py(model_path)
    elif backend == 'cpp':
        return ONNX_model_container_cpp(model_path)


def reset_onnx_shape(onnx_model_path, output_path, input_shapes):
    if isinstance(input_shapes[0], int):
        command = "python -m onnxsim {} {} --input-shape {}".format(onnx_model_path, output_path, ','.join([str(v) for v in input_shapes]))
    else:
        if len(input_shapes)!= 1:
            print("RESET ONNX SHAPE with more than one input, try to match input name")
            sess = rt.InferenceSession(onnx_model_path)
            input_names = [input.name for input in sess.get_inputs()]
            command = "python -m onnxsim {} {} --input-shape ".format(onnx_model_path, output_path)
            for i, input_name in enumerate(input_names):
                command += "{}:{} ".format(input_name, ','.join([str(v) for v in input_shapes[i]]))
        else:
            command = "python -m onnxsim {} {} --input-shape {}".format(onnx_model_path, output_path, ','.join([str(v) for v in input_shapes[0]]))
    
    print(command)
    os.system(command)
    return output_path
    