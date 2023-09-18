import copy
import os
import sys
from ruamel import yaml
from collections import OrderedDict

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')]))

from common.macro_define.rknpu import *


def _dict_to_str(tar_dict):
    _ignore_key = ['img_size']
    _supress_key = ['input_example']
    _supress_lenth = 100
    def _parse_dict(_d, _srt_list, _depth=0):
        _blank = ' '*2
        for _key, _value in _d.items():
            # ignore some key
            if _key in _ignore_key:
                continue

            if isinstance(_value, dict):
                _str.append(_blank*_depth + _key + ':')
                _parse_dict(_value, _srt_list, _depth+1)
            elif _value is None:
                continue
            else:
                _srt_list.append(_blank*_depth + _key + ': ' + str(_value))
                if _key in _supress_key:
                    _srt_list[-1] = _srt_list[-1][:_supress_lenth] + '  ...'

    if not isinstance(tar_dict, dict):
        print('{} is not a dict'.format(tar_dict))

    _str = []
    _parse_dict(tar_dict, _str)
    return _str


def get_example_img_from_dataset(dataset_path, quantize_status):
    with open(dataset_path, 'r') as f:
        contents = f.readlines()
    examples = []
    dataset_dir = os.path.dirname(dataset_path)

    for _path in contents:
        _path = _path.rstrip('\n')
        inputs_list = _path.split(' ')

        # check exist
        for i in range(len(inputs_list)):
            _path = copy.deepcopy(inputs_list[i])
            if not os.path.isabs(_path):
                _path = os.path.join(dataset_dir, _path)
                inputs_list[i] = _path
            if not os.path.exists(_path):
                if quantize_status is True:
                    assert False, '|| {} || input path in dataset.txt not exist'.format(_path)
                elif quantize_status is False:
                    print("|| {} || input path in dataset.txt not exist, compute convert loss will be failed".format(_path))
                    continue
        examples.append(inputs_list)
    return examples


DEFAULT_CVT_CONFIG = {
    "TOOLKIT_MAIN_VERSION": 2,
    "RK_device_platform": "RK3588",
    "RK_device_id": None,
    "model_framework": None,

    "verbose": False,
    "verbose_file": None,
    "quantize": False,
    "dataset": None,

    "graph": {},
    "config": {},

    "pre_compile": "off",           # off, online. only valid for Toolkit1
    
    "core_mask": 1,                 # now only valid for RK3588
                                    # 1 = 0,0,1     -   single-core
                                    # 2 = 0,1,0     -   single-core
                                    # 3 = 0,1,1     -   dual-core
                                    # 4 = 1,0,0     -   single-core
                                    # 7 = 1,1,1     -   triple-core
    "input_example": None,
    

    # PATH param
    "export_rknn": {"export_path": None},
    
    # pytorch, onnx, tensorflow, tflite
    "model_file_path": None,
    # caffe
    "prototxt_file_path": None,
    "caffemodel_file_path": None,
    # darknet
    "cfg_file_path": None,
    "weight_file_path": None,
    # mxnet
    "json_file_path": None,
    "params_file_path": None
}


class RKNN_config_container:
    def __init__(self, config, set_platform='AUTO') -> None:
        self.set_platform=set_platform
        if isinstance(config, str):
            self.yml_file_path = config
            with open(config, 'r') as f:
                # user_define_config = yaml.load(f)
                user_define_config = yaml.safe_load(f)
        else:
            user_define_config = copy.deepcopy(config)

        self.user_define_config = user_define_config
        self.parser_refine_config = copy.deepcopy(DEFAULT_CVT_CONFIG)
        self._project_config_check()
        self._remove_dump_config()
        self.parser_refine_config.update(self.user_define_config)

        self.TOOLKIT_MAIN_VERSION = MACRO_toolkit_version(self.parser_refine_config["RK_device_platform"])
        self.parser_refine_config['TOOLKIT_MAIN_VERSION'] = self.TOOLKIT_MAIN_VERSION
        self.init_config()


    def _project_config_check(self):
        must_exist_key = ["RK_device_platform", "model_framework"]
        for _k in must_exist_key:
            assert _k in self.user_define_config, "{} must set".format(_k, )

        if self.set_platform != 'AUTO':
            self.user_define_config["RK_device_platform"] = self.set_platform.upper()

        self.user_define_config["RK_device_platform"] = self.user_define_config["RK_device_platform"].upper()
        self.user_define_config["model_framework"] = self.user_define_config["model_framework"].lower()

        # compatibale with elder_version
        if 'configs' in self.user_define_config and 'config' not in self.user_define_config:
            print("WARNING - 'configs' in yaml file is deprecate, rename it as 'config'")
            self.user_define_config['config'] = copy.deepcopy(self.user_define_config['configs'])
            self.user_define_config.pop('configs')

        for k, v in self.user_define_config.items():
            if k not in self.parser_refine_config:
                print("WARNING - '{}: {}' is not define in default config".format(k, v))

        RK_device_id = self.user_define_config.get('RK_device_id', None)
        if (RK_device_id is not None) and \
            isinstance(RK_device_id, str) is False: 
            self.user_define_config['RK_device_id'] = str(RK_device_id)

        self._rknn_config_check()


    def _rknn_config_check(self):
        src_config = self.user_define_config.get('config', None)
        if src_config is None:
            return

        try:
            from rknn.api import RKNN
            rknn = RKNN()
            _co_name = rknn.config.__code__.co_names
            remove_key = []
            for _k in src_config:
                if _k not in _co_name:
                    print("WARNING - '{}' is not valid int rknn.config. REMOVED".format(_k))
                    remove_key.append(_k)
            for _k in remove_key:
                src_config.pop(_k)
        except:
            print("WARNING - rknn.api import failed, skip rknn_config_check")


    def _remove_dump_config(self):
        self._remove_dump_model_path()

        removable_key = ["input_example"] 
        for _rm_k in removable_key:
            if _rm_k not in self.user_define_config and \
                _rm_k in self.parser_refine_config:
                self.parser_refine_config.pop(_rm_k)


    def _remove_dump_model_path(self):
        model_framework = self.user_define_config["model_framework"]
        
        general_key = ['model_file_path']
        caffe_key = ['prototxt_file_path', 'caffemodel_file_path']
        darknet_key = ['cfg_file_path', 'weight_file_path']
        mxnet_key = ['json_file_path', 'params_file_path']
        path_key_all = general_key + caffe_key + darknet_key + mxnet_key

        framework_path_keymap = {
            'caffe':        caffe_key,
            'darknet':      darknet_key,
            'mxnet':        mxnet_key,
            'onnx':         general_key,
            'tensorflow':   general_key,
            'tflite':       general_key,
            'pytorch':      general_key,
        }

        assert model_framework in framework_path_keymap, "{} is not support.\nNow support {}".format(model_framework, framework_path_keymap.keys())
        maintain_key = framework_path_keymap[model_framework]
        for _k in path_key_all:
            if _k not in maintain_key and _k in self.parser_refine_config:
                self.parser_refine_config.pop(_k)

    def init_config(self):
        default_config = {}
        if self.TOOLKIT_MAIN_VERSION == 1:
            default_config['quantized_dtype'] = 'asymmetric_affine-u8' # For NPU-1: asymmetric_affine-u8, dynamic_fixed_point-i8, dynamic_fixed_point-i16
        elif self.TOOLKIT_MAIN_VERSION == 2:
            default_config['quantized_dtype'] = 'asymmetric_quantized-8'
        default_config['target_platform'] = self.user_define_config['RK_device_platform']

        if 'config' not in self.user_define_config:
            self.user_define_config['config'] = {}
        for _k, _v in self.user_define_config.items():
            if _k == 'config':
                self.parser_refine_config[_k] = default_config
                self.parser_refine_config[_k].update(_v)
            else:
                if _k not in self.parser_refine_config:
                    print("WARNING - {}: {} is not recogize, this may lead to error".format(_k, _v))
                self.parser_refine_config[_k] = _v


    def gen_build_config(self):
        if hasattr(self, 'yml_file_path'):
            model_path_dir = os.path.dirname(self.yml_file_path)
        else:
            model_path_dir = ''
        build_config = {}
        build_config['do_quantization'] = self.parser_refine_config['quantize']
        build_config['dataset'] = os.path.join(model_path_dir, self.parser_refine_config['dataset'])
        self.parser_refine_config['build'] = build_config

    def gen_model_load_config(self):
        if hasattr(self, 'yml_file_path'):
            model_path_dir = os.path.dirname(self.yml_file_path)
        else:
            model_path_dir = ''

        model_framework = self.parser_refine_config['model_framework']
        model_load_dict = {}
        if 'model_file_path' in self.parser_refine_config:
            # for onnx/pytorch/tensorflow/tflite/keras
            if isinstance(self.parser_refine_config['model_file_path'], list):
                model_load_dict['model'] = [os.path.join(model_path_dir, _p) for _p in self.parser_refine_config['model_file_path']]
            else:
                model_load_dict['model'] = os.path.join(model_path_dir, self.parser_refine_config['model_file_path'])
            if model_framework == 'pytorch':
                self.parser_refine_config['qnnpack'] = self.parser_refine_config.get('qnnpack','False')

        if model_framework == 'caffe':
            model_load_dict['model'] = os.path.join(model_path_dir, self.parser_refine_config['prototxt_file_path'])
            model_load_dict['blobs'] = os.path.join(model_path_dir, self.parser_refine_config['caffemodel_file_path'])
            if self.TOOLKIT_MAIN_VERSION == 1:
                model_load_dict['proto'] = 'caffe'
        elif model_framework == 'darknet':
            model_load_dict['model'] = os.path.join(model_path_dir, self.parser_refine_config['cfg_file_path'])
            model_load_dict['weight'] = os.path.join(model_path_dir, self.parser_refine_config['weight_file_path'])
        elif model_framework == 'mxnet':
            model_load_dict['symbol'] = os.path.join(model_path_dir, self.parser_refine_config['json_file_path'])
            model_load_dict['params'] = os.path.join(model_path_dir, self.parser_refine_config['params_file_path'])
        elif model_framework == 'tensorflow':
            model_load_dict.pop('model')
            model_load_dict['tf_pb'] = os.path.join(model_path_dir, self.parser_refine_config['model_file_path'])

        self.parser_refine_config['load'] = model_load_dict

    def parse_graph(self):
        user_define_config = self.user_define_config
        graph = user_define_config.get('graph', None)
        assert graph != None, "graph should be define in config file"

        # graph input
        inputs_info = OrderedDict()
        if 'in' in graph:
            assert ('in_0' in graph)^('in' in graph), "if graph has multi in, use in_0, in_1 etc."
            inputs_info['in'] = graph['in']
        else:
            start_index = 0
            while True:
                _name = 'in_{}'.format(start_index)
                if _name in graph:
                    inputs_info[_name] = graph[_name]
                    start_index+=1
                else:
                    break
        self.parser_refine_config['inputs'] = inputs_info

        # graph output
        outputs_info = OrderedDict()
        if 'out' in graph:
            assert ('out_0' in graph)^('out' in graph), "if graph has multi out, use out_0, out_1 etc."
            outputs_info['out'] = graph['out']
        else:
            start_index = 0
            while True:
                _name = 'out_{}'.format(start_index)
                if _name in graph:
                    outputs_info[_name] = graph[_name]
                    start_index+=1
                else:
                    break
        self.parser_refine_config['outputs'] = outputs_info

    def generate_random_input(self):
        print("TRYING generate random input")
        fake_in_dir = "./tmp/fake_in"
        if not os.path.exists(fake_in_dir):
            os.makedirs(fake_in_dir)
        name_list = []
        for key, item in self.parser_refine_config['graph'].items():
            shape = item['shape']
            print("  generate for {} with shape - {}".format(key, shape))
            import numpy as np
            fake_in = np.random.rand(*[int(v) for v in str(shape).split(',')]).astype(np.float32)
            # fake_in = np.random.randint(0,255,tuple([int(v) for v in shape.split(',')])).astype(np.float)
            fake_name = os.path.join(fake_in_dir, key + '.npy')
            np.save(fake_name, fake_in)
            name_list.append(os.path.basename(fake_name))

        fake_dataset = os.path.join(fake_in_dir, "fake_dataset.txt")
        with open(fake_dataset, 'w') as f:
            f.write(' '.join(name_list)) 

        self.parser_refine_config['dataset'] = fake_dataset
        self.parser_refine_config['build']['dataset'] = fake_dataset

    def update_dataset(self):
        inputs_example = self.parser_refine_config.get('input_example', None)
        if inputs_example is None:
            if self.parser_refine_config['dataset'] is not None:
                multi_examples = get_example_img_from_dataset(self.parser_refine_config['dataset'], self.parser_refine_config['build']['do_quantization'])
            else:
                return 
        else:
            multi_examples = inputs_example.split(' ')
        
        inputs_path_list_with_port = []
        inputs_key = list(self.parser_refine_config['inputs'].keys())
        for single_example in multi_examples:
            _temp_dict = {}
            for i in range(len(inputs_key)):
                _temp_dict[inputs_key[i]] = single_example[i]
            inputs_path_list_with_port.append(_temp_dict)

        self.parser_refine_config['input_example'] = inputs_path_list_with_port


    def update_graph(self):
        self.user_define_config['no_scale_values'] = True

        mean_values_list = []
        std_values_list = []
        input_size_list = []
        reorder_list = []       # For toolkit1
        channel_type_list = []  # For toolkit2
        inputs_name = []
        outputs_name = []

        graph_update_dict = {}

        for key, sub_dict in self.parser_refine_config['inputs'].items():
            assert 'shape' in sub_dict, "shape must be define for {}".format(key)
            # record node name if available
            _input_name = sub_dict.get('name', None)
            if _input_name is not None:
                inputs_name.append(_input_name)

            # input_size_list
            input_size_list.append([int(_v) for _v in str(sub_dict['shape']).split(',')])

            if len(input_size_list[-1]) == 1:
                channel_dim_size = 1
            else:
                if self.parser_refine_config['model_framework'] in ['tensorflow', 'tflite', 'keras']:
                    channel_dim_size = input_size_list[-1][-1]
                else:
                    channel_dim_size = input_size_list[-1][1]

            if len(input_size_list[-1])!= 4:
                img_size = None
            else:
                if self.parser_refine_config['model_framework'] in ['tensorflow', 'tflite', 'keras']:
                    img_size = input_size_list[-1][1:3]
                else:
                    img_size = input_size_list[-1][2:]
            
            # mean_values
            mean_values = sub_dict.get('mean_values', 0)
            if mean_values!= 0:
                self.user_define_config['no_scale_values'] = False
            if isinstance(mean_values, str):
                mean_values_list.append([float(_v) for _v in mean_values.split(',')])
            else:
                mean_values_list.append([mean_values]* channel_dim_size)

            # std_values
            std_values = sub_dict.get('std_values', 1)
            if std_values!= 1:
                self.user_define_config['no_scale_values'] = False
            if isinstance(std_values, str):
                std_values_list.append([float(_v) for _v in std_values.split(',')])
            else:
                std_values_list.append([std_values]* channel_dim_size)

            # reorder_channel
            channel_type = sub_dict.get('img_type', None)
            if channel_type is None:
                #! not allow some input with img_type and other without img_type
                if (len(input_size_list[-1]) ==4 and input_size_list[-1][1] == 1):
                    channel_type = 'GRAY' 
                elif (len(input_size_list[-1]) ==4 and input_size_list[-1][1] == 3):
                    channel_type = 'RGB'
                # elif 
                channel_type_list = False
            else:
                if channel_type == 'BGR':
                    channel_type_list.append(True)
                    reorder_list.append('2 1 0')
                elif channel_type == 'RGB':
                    channel_type_list.append(False)
                    reorder_list.append('0 1 2')
                else:
                    assert False, "only RGB or BGR support, got img_type-{}".format(channel_type)

            # update parser_refine_config['inputs'] dict
            _input_update_dict = {}
            _input_update_dict['shape'] = input_size_list[-1]
            _input_update_dict['mean_values'] = mean_values_list[-1]
            _input_update_dict['std_values'] = std_values_list[-1]
            _input_update_dict['img_type'] = channel_type
            _input_update_dict['img_size'] = img_size
            graph_update_dict[key] = _input_update_dict

        self.parser_refine_config['inputs'] = graph_update_dict

        for key, sub_dict in self.parser_refine_config['outputs'].items():
            outputs_name.append(sub_dict['name'])
            
        # update to real position
        self.parser_refine_config['config']['mean_values'] = mean_values_list
        self.parser_refine_config['config']['std_values'] = std_values_list
        if self.TOOLKIT_MAIN_VERSION == 1:
            if len(reorder_list) == 0:
                self.parser_refine_config['config']['reorder_channel'] = None
            else:
                self.parser_refine_config['config']['reorder_channel'] = '#'.join(reorder_list)
        elif self.TOOLKIT_MAIN_VERSION == 2:
            self.parser_refine_config['config']['quant_img_RGB2BGR'] = channel_type_list

        # TODO if other model_framework support load subgraph, add here 
        if self.parser_refine_config['model_framework'] in ['tensorflow', 'onnx'] and \
            inputs_name != [] and outputs_name != []:
            pass
            # TODO support
            self.parser_refine_config['load']['inputs'] = inputs_name
            self.parser_refine_config['load']['outputs'] = outputs_name

        # model_framework need input size
        if self.parser_refine_config['model_framework'] in ['pytorch', 'tensorflow', 'mxnet']:
            if self.TOOLKIT_MAIN_VERSION == 1:
                _input_size_list = copy.deepcopy(input_size_list)
                for i, size in enumerate(_input_size_list):
                    if len(size)==4 and size[0]==1:
                        _input_size_list[i] = size[1:]
                self.parser_refine_config['load']['input_size_list'] = _input_size_list
            elif self.TOOLKIT_MAIN_VERSION == 2:
                self.parser_refine_config['load']['input_size_list'] = input_size_list

    def update_path(self, path_type, given_path):
        model_name = self.user_define_config.get('model_name', None)
        platform = PLATFROM_COMPATIBAL_TRANSFORMER_MAP[self.user_define_config['RK_device_platform'].upper()]
        
        if model_name is None:
            model_name = 'model'
            src_model_name = self.parser_refine_config['load'].get('model', None)
            if src_model_name is not None:
                if isinstance(src_model_name, list):
                    src_model_name = src_model_name[0]
                    src_model_name = src_model_name.split(os.sep)[-1]
                    model_name = '.'.join(src_model_name.split('.')[0:-1]) + "_dyn_shape"
                else:
                    src_model_name = src_model_name.split(os.sep)[-1]
                    model_name = '.'.join(src_model_name.split('.')[0:-1])
        else:
            model_name = os.path.basename(model_name)
            if '.' in model_name:
                elements = model_name.split('.')
                if elements[-1] in ['om', 'omx', 'onnx', 'pb', 'pt', 'pth', 'h5', 'caffemodel', 'prototxt']:
                    model_name = '.'.join(elements[0:-1])

        dtype = MACRO_qtype_toolkit_2_sim(self.parser_refine_config['config']['quantized_dtype']) if self.user_define_config.get('quantize', False) else 'fp'
        model_name = '_'.join([model_name, 
                               platform,
                               dtype,
                              ])
        if 'remove_weight' in self.parser_refine_config['config'] and \
            self.parser_refine_config['config']['remove_weight'] == True:
            model_name = model_name + '_remove_weight'

        if 'dynamic_input' in self.parser_refine_config['config'] and \
            self.parser_refine_config['config']['dynamic_input'] is not None:
            model_name = model_name + '_dyn'

        sub_folder = './model_cvt'
        if given_path == 'AUTO':
            _ = os.mkdir(sub_folder) if os.path.exists(sub_folder) is not True else 0
            device_folder = os.path.join(sub_folder, platform)
            _ = os.mkdir(device_folder) if os.path.exists(device_folder) is not True else 0
            given_path = os.path.join(device_folder, model_name)
        elif not given_path.endswith('.rknn'):
            given_path = os.path.join(given_path, model_name)

        self.parser_refine_config['export_rknn']['export_path'] = given_path.rstrip('.rknn') + '.rknn'
        self.parser_refine_config['export_pre_compile_path'] = given_path.rstrip('.rknn') + '_precompile.rknn'

    def print_config(self):
        # TODO optimize print logic
        print('='*10, 'parser_config', '='*10)
        _str = _dict_to_str(self.parser_refine_config)
        for _s in _str:
            print(_s)
        print('='*35)

    def align_quantized_type(self):
        if self.TOOLKIT_MAIN_VERSION == 1 and \
           self.parser_refine_config['config'].get('quantized_dtype') == 'asymmetric_quantized-8':
            self.parser_refine_config['config']['quantized_dtype'] = 'asymmetric_affine-u8'
        elif self.TOOLKIT_MAIN_VERSION == 2 and \
             self.parser_refine_config['config'].get('quantized_dtype') == 'asymmetric_affine-u8':
             self.parser_refine_config['config']['quantized_dtype'] = 'asymmetric_quantized-8'

    def remove_mean_std_if_neednt(self):
        if self.user_define_config.get('no_scale_values', False):
            self.parser_refine_config['config'].pop('mean_values')
            self.parser_refine_config['config'].pop('std_values')
            for key, info in self.parser_refine_config['inputs'].items():
                info.pop('mean_values')
                info.pop('std_values')

    def push_back(self):
        return self.parser_refine_config

    def parse(self, generate_random_input=True):
        self.gen_build_config()
        self.gen_model_load_config()
        self.parse_graph()

        self.update_graph()

        if generate_random_input:
            self.generate_random_input()
        self.update_dataset()
        
        self.align_quantized_type()     # align toolkit1,toolkit2 format
        self.remove_mean_std_if_neednt()

