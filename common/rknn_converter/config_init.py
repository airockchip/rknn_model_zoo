import os
import sys
from ruamel import yaml

realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
# sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')]))
config_folder = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('rknn_converter')+1], 'config_example')

from common.macro_define.rknpu import *

def _dict_to_str(tar_dict):
    def _parse_dict(_d, _srt_list, _depth=0):
        _blank = '  '
        for _key, _value in _d.items():
            if isinstance(_value, dict):
                _str.append(_blank*_depth + _key + ':\n')
                _parse_dict(_value, _srt_list, _depth+1)
            else:
                _srt_list.append(_blank*_depth + _key + ': ' + str(_value) + '\n')

    if not isinstance(tar_dict, dict):
        print('{} is not a dict'.format(tar_dict))

    _str = []
    _parse_dict(tar_dict, _str)
    return _str

framework_mapper = {
    'pytorch': 'pytorch_model_config.yml',
    'torch': 'pytorch_model_config.yml',
    'pt': 'pytorch_model_config.yml',
    'onnx': 'onnx_model_config.yml',
    'tensorflow': 'tensorflow_model_config.yml',
    'tf': 'tensorflow_model_config.yml',
    'tflite': 'tflite_model_config.yml',
    'keras': 'keras_model_config.yml',
    'caffe': 'caffe_model_config.yml',
    'darknet': 'darknet_model_config.yml',
    'mxnet': 'mxnet_model_config.yml',
}

if __name__ == '__main__':
    if len(sys.argv) < 2 or \
        len(sys.argv)==2 and sys.argv[1] in ['--help', '-help', '-h', '--h']:
        print("usage: python config_init.py [source_framework] [device_platform] [quantized_dtype]")
        print("  available framework:\n    {}".format(list(framework_mapper.keys())))
        print("  available device_platform:\n    {}".format(RKNN_DEVICES_ALL))
        print("  available quantize_type:\n    {}".format(QUANTIZE_DTYPE_SIM))
        exit()

    source_framework = sys.argv[1]
    device_platform = None
    quantized_dtype = None

    device_platform = 'rk1808' if len(sys.argv) < 3 else sys.argv[2]
    quantized_dtype = 'u8' if len(sys.argv) < 4 else sys.argv[3]

    NPU_version = None
    if device_platform.upper() in NPU_VERSION_1_DEVICES:
        NPU_version = 1
    elif device_platform.upper() in NPU_VERSION_2_DEVICES:
        NPU_version = 2
    else:
        assert False,"{} not support".format(device_platform)

    assert quantized_dtype in QUANTIZE_DTYPE_SIM, "quantized_dtype: {} not support".format(quantized_dtype)

    src_config = framework_mapper[source_framework]
    src_config = os.path.join(config_folder, src_config)
    with open(src_config, 'r') as f:
        conf_dict = yaml.load(f)
    

    conf_dict['RK_device_platform'] = device_platform
    if quantized_dtype == 'fp':
        conf_dict['quantize'] = False
    else:
        conf_dict['quantize'] = True
        qtype = MACRO_qtype_sim_2_toolkit(quantized_dtype, device_platform.upper())

        conf_dict['configs']['quantized_dtype'] = qtype

    conf_txt = _dict_to_str(conf_dict)
    out_name = './model_config.yml'
    with open(out_name, 'w') as f:
        for _s in conf_txt:
            f.write(_s)

    print("\ninit finish")