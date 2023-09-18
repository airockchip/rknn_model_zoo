import os
import sys
from copy import copy, deepcopy
import numpy as np

import time
from ruamel import yaml
from rknn.api import RKNN

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]))

from image_utils.img_preprocesser_tools import Image_preprocessor
from image_utils.numpy_preprocesser_tools import numpy_preprocessor
from framework_executor.executor import Excuter
from capi_simply_executor.commond_executor.toolkit1_capi import tk1_capi_executor
from capi_simply_executor.commond_executor.toolkit2_capi import tk2_capi_executor
from utils.dict_tools import _dict_to_str
from utils.shell_utils import check_file, check_devices_available
from macro_define.rknpu import *


class time_collecter:
    def __init__(self) -> None:
        self.times = []

    def tik(self):
        self.times.append(time.time())

    def last_time(self, _f='ms', decimal=2):
        result = self.times[-1] - self.times[-2]
        if _f == 's':
            decimal = max(4, decimal)
        elif _f == 'ms':
            result *= 1000
        result = float('%.{}f'.format(decimal)%result)
        return result

    def all(self):
        pass

    def flash(self):
        self.times = []

def _device_require(func):
    def wrapper(*args, **kwars):
        phase_object = args[0]
        if phase_object.device_connected is False and phase_object.RK_device_id!= 'simulator':
            print("WARNING: no adb devices. Ignore {}".format(func.__name__))
            return
        return func(*args, **kwars)
    return wrapper

def _device_limit(func):
    def wrapper(*args, **kwars):
        phase_object = args[0]
        if phase_object.RK_device_platform in NPU_V2_2 and phase_object.RK_device_id != 'simulator':
            print("WARNING: {} not support run model with python.api. Ignore {}".format(phase_object.RK_device_platform, func.__name__))
            return
        return func(*args, **kwars)
    return wrapper

def _simulator_limit(func):
    def wrapper(*args, **kwars):
        phase_object = args[0]
        if phase_object.RK_device_id == 'simulator':
            print("WARNING: simulator not support run model with c.api. Ignore {}".format(func.__name__))
            return
        return func(*args, **kwars)
    return wrapper

class convert_phase:
    # --------------------
    # Only doing convert here.
    # --------------------
    def __init__(self, model_config_dict , args=None):
        self.config_check(model_config_dict)
        self.model_config_dict = model_config_dict
        self.args = args

    def config_check(self, model_config_dict):
        # TODO
        pass

    def convert(self):
        # do not change model_config_dict here.
        model_config_dict = deepcopy(self.model_config_dict)

        overwrite = True
        if self.args.overwrite.lower() == 'yes':
            pass
        elif self.args.overwrite.lower() == 'no':
            if os.path.exists(model_config_dict['export_rknn']['export_path']) or os.path.exists(model_config_dict['export_pre_compile_path']):
                overwrite = False
        elif self.args.overwrite == 'check':
            if os.path.exists(model_config_dict['export_rknn']['export_path']):
                print('{} is already exists! Do you want to overwrite it? [Y/N]'.format(model_config_dict['export_rknn']['export_path']))
                key_in = ' '
                while key_in not in ['Y', 'N']:
                    key_in = input("Y or N:")
                if key_in == 'N':
                    overwrite = False
                print('\n\n')
        else:
            assert False, "--overwrite {} is not support, only support [yes, no, check]".format(self.args.overwrite)

        # Convert rknn model
        if overwrite is True:
            # TODO hybrid quant
            print('---> Create RKNN object')
            rknn = RKNN(verbose=model_config_dict['verbose'], verbose_file=model_config_dict['verbose_file'])

            print('---> Seting RKNN config')
            rknn.config(**model_config_dict['config'])

            print('---> Loading {} model'.format(model_config_dict['model_framework']))
            load_function = getattr(rknn, 'load_{}'.format(model_config_dict['model_framework']))
            load_function(**model_config_dict['load'])

            print('---> Building')
            rknn.build(**model_config_dict['build'])

            print('---> Export RKNN model')
            rknn.export_rknn(**model_config_dict['export_rknn'])


            if model_config_dict['TOOLKIT_MAIN_VERSION'] != 1:
                print("  WARNING: {} model needn't pre_compile. Ignore!".format(model_config_dict['RK_device_platform']))
            elif model_config_dict['RK_device_id'] == 'simulator':
                print("  WARNING: simulator does not support exporting pre_compiled model. Ignore!")
            elif model_config_dict['pre_compile'] == 'online':
                print('---> Export pre-complie model')
                rknn.init_runtime(target=model_config_dict['RK_device_platform'], device_id = model_config_dict['RK_device_id'], rknn2precompile=True)
                rknn.export_rknn_precompile_model(model_config_dict['export_pre_compile_path'])
                del rknn
                print('---> Reload pre_compile rknn model')
                rknn = RKNN(verbose=model_config_dict['verbose'], verbose_file=model_config_dict['verbose_file'])
                rknn.load_rknn(model_config_dict['export_pre_compile_path'])

        elif overwrite is False:
            rknn = RKNN(verbose=model_config_dict['verbose'], verbose_file=model_config_dict['verbose_file'])
            print('---> Load RKNN model')
            if model_config_dict.get('pre_compile', 'off') == 'online' and \
                os.path.exists(model_config_dict['export_pre_compile_path']):
                rknn.load_rknn(model_config_dict['export_pre_compile_path'])
            else:
                rknn.load_rknn(model_config_dict['export_rknn']['export_path'])
        return rknn


class validate_phase:
    # --------------------
    #  0.init rknn runtime
    #  1.eval memory (python)
    #  2.eval performance (python)
    #  3.eval performance (C API)
    #  4.get rknn result (python)
    #  5.get rknn result (C API)
    #  6.get framework result
    #  7.compare convert dist (python)
    #  8.compare convert dist (C API)
    #  9.compare C API, python result
    # 10.eval memory bandwidth (C API) # TODO

    # 99.get default framework result
    # --------------------
    #  Usage: 
    # 
    #          init runtime before using rknn python api
    # 
    #
    # --------------------
    def __init__(self, rknn, model_config_dict, args) -> None:
        assert isinstance(rknn, RKNN)

        self.device_connected = check_devices_available()

        self.rknn = rknn
        self.model_config_dict = model_config_dict
        self.TOOLKIT_MAIN_VERSION = model_config_dict.get('TOOLKIT_MAIN_VERSION', '1')
        self.RK_device_platform = model_config_dict.get('RK_device_platform', 'RK1808')
        self.RK_device_id = model_config_dict.get('RK_device_id', 'simulator')

        self.init_level_order = ['normal', 'eval_mem', 'perf_debug']
        self.init_state = None

        self.framework_excute_info = None
        self.framework_runner = None

        self.capi_executor = None

        self.model_type = 'fp'
        if model_config_dict['build']['do_quantization']:
            self.model_type = model_config_dict['config']['quantized_dtype']

        # for recording
        self._report_available = getattr(args, "report", False)
        script = '/'.join(realpath[:-1] + ["report_script.yml"])
        with open(script, 'r') as f:
            self.report_info = yaml.safe_load(f)
        self._init_convinient_key_map()
        self.tc = time_collecter()

        # init and get board info
        test_api = ['capi_test', 'capi_zero_copy_test', 'eval_perf', 'eval_memory', 'python_api_test']
        test_api_status = [getattr(args, _a) for _a in test_api]
        if len(set(test_api_status))>1 or test_api_status[0]!= False:
            self.check_board_info()

    @_device_require
    @_device_limit
    def init_rknn_runtime(self, perf_debug=False, eval_mem=False):
        # do not change model_config_dict here.
        model_config_dict = deepcopy(self.model_config_dict)
        assert not perf_debug&eval_mem , 'perf_debug and eval_mem must not be True at the same time.'
        if perf_debug is True:
            want_state = 'perf_debug'
        elif eval_mem is True:
            want_state = 'eval_mem'
        else:
            want_state = 'normal'

        if self.init_state != want_state:
            print('---> init_rknn_runtime')
            if model_config_dict['RK_device_id'] != 'simulator':
                self.tc.tik()
                ret = self.rknn.init_runtime(target=model_config_dict['RK_device_platform'], device_id = model_config_dict['RK_device_id'], perf_debug=perf_debug, eval_mem=eval_mem)
                self.tc.tik()
                if (self._report_available is True) and (perf_debug is False) and (eval_mem is False):
                    self._smart_record('Python_api.time_cost(ms).init', self.tc.last_time())
            else:
                ret = self.rknn.init_runtime()
            self.init_state = want_state
            if ret != 0:
                assert False, 'init_rknn_runtime failed!'

    @_device_require
    @_device_limit
    @_simulator_limit
    def eval_memory(self):
        print('\n\n', '='*30)
        print('Eval memory\n')
        self.init_rknn_runtime(perf_debug=False, eval_mem=True)
        memory_info = self.rknn.eval_memory()
        # TODO parse memory_info format
        if self._report_available is True:
            if self.TOOLKIT_MAIN_VERSION == 1:
                weight = "%.2f" % (memory_info['system_memory']['maximum_allocation']/1024/1024)
                internal = "%.2f" % (memory_info['npu_memory']['maximum_allocation']/1024/1024)
                total = "%.2f" % (memory_info['total_memory']['maximum_allocation']/1024/1024)
            if self.TOOLKIT_MAIN_VERSION == 2:
                weight = "%.2f" % (memory_info['total_weight_allocation']/1024/1024)
                internal = "%.2f" % (memory_info['total_internal_allocation']/1024/1024)
                total = "%.2f" % (memory_info['total_model_allocation']/1024/1024)

            self._smart_record('Memory_info(MiB).weight', weight)
            self._smart_record('Memory_info(MiB).internal', internal)
            self._smart_record('Memory_info(MiB).total', total)
            if self.model_config_dict.get('pre_compile', 'off') == 'online' and \
                os.path.exists(self.model_config_dict['export_pre_compile_path']):
                _m_path = self.model_config_dict['export_pre_compile_path']
            else:
                _m_path = self.model_config_dict['export_rknn']['export_path']
            self._smart_record('Memory_info(MiB).model_file_size', check_file(_m_path, 'size'))
        return memory_info

    @_device_require
    @_device_limit
    @_simulator_limit
    def eval_perf(self, looptime=5, perf_debug=False):
        print('\n\n', '='*30)
        print('Eval perf\n  looptime: {}\n  perf_debug: {}'.format(looptime, perf_debug))
        self.init_rknn_runtime(perf_debug=perf_debug, eval_mem=False)
        perf_info = self.rknn.eval_perf(looptime)
        # TODO parse memory_info format
        if self._report_available is True:
            if self.TOOLKIT_MAIN_VERSION == 1:
                self._smart_record("eval_performance(only model inference)", "%.2f"%(perf_info['total_time']/1000))
            elif self.TOOLKIT_MAIN_VERSION == 2:
                time = perf_info.split("Time(us): ")[-1].split('\n')[0]
                self._smart_record("eval_performance(only model inference)", float(time)/1000)
        return perf_info

    @_device_require
    @_device_limit
    def get_rknn_result_via_python(self, inputs):
        self.init_rknn_runtime(perf_debug=False, eval_mem=False)
        self.tc.tik()
        # for i, _in in enumerate(inputs):
        #     if (len(_in.shape) in [3, 4]) and _in.shape[-1] != 3:
        #         # nchw 2 nhwc
        #         _perm = (0, 2, 3, 1) if len(_in.shape) == 4 else (1, 2, 0)
        #         inputs[i] = np.transpose(_in, _perm)
        rknn_result = self.rknn.inference(inputs)
        self.tc.tik()
        if self._report_available:
            self._smart_record('run(include data transmission)', self.tc.last_time())
        return rknn_result

    @_device_require
    @_simulator_limit
    def get_rknn_result_via_Capi(self, inputs, looptime=5, api_type='normal'):
        self.release()
        if self.TOOLKIT_MAIN_VERSION == 1:
            result, time_info = self._get_rknn_result_via_Capi_npu1(inputs, looptime, api_type)
        elif self.TOOLKIT_MAIN_VERSION == 2:
            result, time_info = self._get_rknn_result_via_Capi_npu2(inputs, looptime, api_type)

        print('---> time_info:')
        for key, value in time_info.items():
            print('  {}: {} ms'.format(key, value))
        return result, time_info

    @_device_require
    def _get_rknn_result_via_Capi_npu1(self, inputs, looptime, api_type):
        if self.capi_executor is None:
            if self.model_config_dict['pre_compile'] == 'online':
                capi_executor = tk1_capi_executor(self.model_config_dict['export_pre_compile_path'], deepcopy(self.model_config_dict), getattr(self, 'device_system'))
            else:
                capi_executor = tk1_capi_executor(self.model_config_dict['export_rknn']['export_path'], deepcopy(self.model_config_dict), getattr(self, 'device_system'))
            self.capi_executor = capi_executor
        result, time_info = self.capi_executor.execute(inputs, looptime, api_type)
        return result, time_info

    @_device_require
    def _get_rknn_result_via_Capi_npu2(self, inputs, looptime, api_type):
        if self.capi_executor is None:
            self.capi_executor = tk2_capi_executor(self.model_config_dict['export_rknn']['export_path'], deepcopy(self.model_config_dict), getattr(self, 'device_system', 'android'))
        result, time_info = self.capi_executor.execute(inputs, looptime, api_type)
        return result, time_info


    def get_default_framework_result(self, inputs):
        model_config_dict = deepcopy(self.model_config_dict)
        if self.framework_excute_info is None:
            self.framework_excute_info = self._get_framework_info()
        if self.framework_runner is None:
            self.framewok_runner = Excuter(self.framework_excute_info)
        
        framework = model_config_dict['model_framework']
        print('---> compute {} result'.format(framework))
        framework_result = self.framewok_runner.inference(inputs)

        self.model_config_dict['output_shape_by_run'] = [list(_v.shape) for _v in framework_result]
        return framework_result

    @_device_require
    @_device_limit
    def compare_convert_dist_via_python(self, sample=1):
        print('\n\n', '='*30)
        print('compare_convert_dist_via_python\n  sample: {}'.format(sample))

        model_config_dict = deepcopy(self.model_config_dict)
        framework = model_config_dict['model_framework']

        if sample > len(self.model_config_dict['input_example']):
            print('sample-{} excced example in dataset, reset to upper bound-{}'.format(sample, len(self.model_config_dict['input_example'])))
            sample = len(self.model_config_dict['input_example'])

        result_info = []
        for i in range(sample):
            rk_inputs = self._get_input('rknn', i)
            rknn_result = self.get_rknn_result_via_python(rk_inputs)

            framework_inputs = self._get_input('origin_framework', i)
            framework_result = self.get_default_framework_result(framework_inputs)

            print('     For example iter {}/{} rknn({}) VS {} cos_similarity:'.format(i+1, sample, self.model_type, framework))
            result_info.extend(self._parse_result(rknn_result, framework_result))

        if self._report_available:
            # TODO support custom output name
            for i in range(len(result_info)):
                self.report_info['Model_info']['Python_api']['accuracy(cos simularity)']['output_{}'.format(i)] = result_info[i]

        return result_info

    @_device_require
    @_simulator_limit
    def compare_convert_dist_via_Capi(self, looptime=5, api_type='normal', sample=1):
        print('\n\n', '='*30)
        if self.RK_device_platform.upper() == 'RK3399PRO' and api_type == 'zero_copy':
            print("WARNING: RK3399PRO not support zero copy. Ignore zero copy test")
            return

        if self.RK_device_platform.upper() in ['RV1106', 'RV1103'] and api_type == 'normal':
            print("WARNING: {} only support capi zero copy, Ignore {} test".format(self.RK_device_platform.upper(), api_type))
            return

        print('compare_convert_dist_via_Capi\n  looptime: {}\n  api_type: {}\n  sample: {}'.format(looptime, api_type, sample))

        model_config_dict = deepcopy(self.model_config_dict)
        framework = model_config_dict['model_framework']

        if sample > len(self.model_config_dict['input_example']):
            print('sample-{} excced example in dataset, reset to upper bound-{}'.format(sample, len(self.model_config_dict['input_example'])))
            sample = len(self.model_config_dict['input_example'])

        result_info = []
        for i in range(sample):
            rk_inputs = self._get_input('rknn', i)
            rknn_result, time_info = self.get_rknn_result_via_Capi(rk_inputs, looptime, api_type)

            framework_inputs = self._get_input('origin_framework', i)
            framework_result = self.get_default_framework_result(framework_inputs)

            print('     For example iter {}/{} rknn({}) VS {} cos_similarity:'.format(i+1, sample, self.model_type, framework))
            result_info.extend(self._parse_result(rknn_result, framework_result))

        if self._report_available:
            # TODO support custom output name
            for i in range(len(result_info)):
                self.report_info['Model_info']['RKNN_api({})'.format(api_type)]['accuracy(cos simularity)']['output_{}'.format(i)] = result_info[i]
            
            if api_type == 'normal':
                self._smart_record('RKNN_api(normal).time_cost(ms).model_init', time_info['model_init'])
                self._smart_record('RKNN_api(normal).time_cost(ms).input_set', time_info['input_set'])
                self._smart_record('RKNN_api(normal).time_cost(ms).run', time_info['run'])
                self._smart_record('RKNN_api(normal).time_cost(ms).output_get', time_info['output_get'])
                self._smart_record('RKNN_api(normal).time_cost(ms).total(except init)', time_info['output_get']+time_info['run']+time_info['input_set'])
            elif api_type == 'zero_copy':
                self._smart_record('RKNN_api(zero_copy).time_cost(ms).model_init', time_info['model_init'])
                self._smart_record('RKNN_api(zero_copy).time_cost(ms).input_io_init', time_info['input_io_init'])
                self._smart_record('RKNN_api(zero_copy).time_cost(ms).output_io_init', time_info['output_io_init'])
                self._smart_record('RKNN_api(zero_copy).time_cost(ms).run', time_info['run'])
                self._smart_record('RKNN_api(zero_copy).time_cost(ms).total(except init)', time_info['run'])
        return result_info, time_info

    @_device_require
    @_simulator_limit
    def compare_rknn_python_Capi_dist(self, sample):
        print('\n\n', '='*30)
        print('compare_rknn_python_Capi_dist\n  sample: {}'.format(sample))
        model_config_dict = deepcopy(self.model_config_dict)


        if sample > len(self.model_config_dict['input_example']):
            print('sample-{} excced example in dataset, reset to upper bound-{}'.format(sample, len(self.model_config_dict['input_example'])))
            sample = len(self.model_config_dict['input_example'])

        result_info = []
        for i in range(sample):
            rk_inputs = self._get_input('rknn', i)
            rknn_result_python = self.get_rknn_result_via_python(rk_inputs)
            rknn_result_Capi_normal, time_info = self.get_rknn_result_via_Capi(rk_inputs, looptime=1, api_type='normal')
            # rknn_result_Capi_zcp, time_info = self.get_rknn_result_via_Capi(rk_inputs, looptime=1, api_type='zero_copy')

            print('     For example iter {}/{} rknn({})(python) VS rknn({})(Capi) cos_similarity:'.format(i+1, sample, self.model_type, self.model_type))
            result_info.extend(self._parse_result(rknn_result_python, rknn_result_Capi_normal))


    def _get_input(self, framework='origin_framework', example_index=0):
        if framework == 'origin_framework':
            if self.framework_excute_info is None:
                self._get_framework_info()
            framework = self.framework_excute_info['model_framework']

        model_config_dict = deepcopy(self.model_config_dict)
        single_example = model_config_dict['input_example'][example_index]

        input_list = []
        for _index, input_info in model_config_dict['inputs'].items():
            #! when input RGB/BGR not match, probobly the this code get wrong type
            _img_type = 'RGB' if (framework.upper() == 'RKNN' and self.TOOLKIT_MAIN_VERSION==1 and input_info['img_type'] in ['RGB','BGR']) else input_info['img_type'] # rknn_toolkit_1 always get RGB in.

            if single_example[_index].endswith('.npy'):
                imp = numpy_preprocessor(single_example[_index])
                imp.check_and_reshape(input_info['shape'])
            else:
                imp = Image_preprocessor(single_example[_index], _img_type)    # rknn_toolkit_1 always get RGB in.
                imp.resize(input_info['img_size'])
                if framework not in ['caffe','darknet','rknn']:
                    if 'mean_values' in input_info:
                        imp.normalize(input_info['mean_values'], input_info['std_values'])
                    else:
                        imp.to_float()
            input_list.append(imp.get_input(framework))

        return input_list


    def _get_framework_info(self):
        model_config_dict = deepcopy(self.model_config_dict)
        if self.framework_excute_info is None:
            framework_excute_info = {}
            framework_excute_info['model_framework'] = model_config_dict['model_framework']
            if model_config_dict['model_framework'] == 'tensorflow':
                framework_excute_info['model'] = model_config_dict['load']['tf_pb']
                framework_excute_info['input_nodes'] = model_config_dict['load']['inputs']
                framework_excute_info['output_nodes'] = model_config_dict['load']['outputs']

            elif model_config_dict['model_framework'] == 'caffe':
                framework_excute_info['prototxt'] = model_config_dict['load']['model']
                framework_excute_info['caffemodel'] = model_config_dict['load']['blobs']
                framework_excute_info['output_nodes'] = []
                for _out, _value in model_config_dict['outputs'].items():
                    framework_excute_info['output_nodes'].append(_value['name'])
                for _index, input_info in model_config_dict['inputs'].items():
                    #! only support single input now
                    framework_excute_info['mean_values'] = input_info.get('mean_values', None)
                    framework_excute_info['std_values'] = input_info.get('std_values', None)

            elif model_config_dict['model_framework'] == 'mxnet':
                framework_excute_info['params'] = model_config_dict['load']['params']
                framework_excute_info['symbol'] = model_config_dict['load']['symbol']

            elif model_config_dict['model_framework'] == 'darknet':
                framework_excute_info['model'] = model_config_dict['load']['model']
                framework_excute_info['weight'] = model_config_dict['load']['weight']
                for _index, input_info in model_config_dict['inputs'].items():
                    #! only support single input now
                    framework_excute_info['mean_values'] = input_info['mean_values']
                    framework_excute_info['std_values'] = input_info['std_values']

            else: # onnx/pytorch/tflite
                framework_excute_info['model'] = model_config_dict['load']['model']
                if model_config_dict['model_framework'] == 'pytorch':
                    framework_excute_info['qnnpack'] = model_config_dict['qnnpack']
            self.framework_excute_info = framework_excute_info


    def _compare_cos_simularity(self, x, y):
        x = x.flatten()
        y = y.flatten()
        cos_dist= (x* y)/(np.linalg.norm(x)*(np.linalg.norm(y)))
        return cos_dist.sum()

    def _parse_result(self, x, y, indent=8):
        msg_list = []
        for i in range(len(x)):
            msg_list.append(self._compare_cos_simularity(x[i], y[i]))
            if x[i].size == 1:
                msg_list[-1]="single value has no meaning in cos. Got {} vs {}".format(x[i], y[i])
            print(" "*indent + "output-{} : {}".format(i, msg_list[-1]))
        return msg_list

    def release(self):
        if self.init_state is not None:
            self.rknn.release()
        self.init_state = None

    @_device_require
    @_simulator_limit
    def check_board_info(self):
        from utils.board_checker import Board_checker
        if self.model_config_dict['RK_device_id'] == 'simulator' or check_devices_available() is False:
            return
        bc = Board_checker(self.model_config_dict['RK_device_platform'], self.model_config_dict['RK_device_id'])
        self._smart_record('chipname', self.model_config_dict['RK_device_platform'])
        self._smart_record('system', bc.get_device_system_type())
        setattr(self, 'device_system', bc.get_device_system_type())
        self._smart_record('librknn_runtime_version', "{}".format(bc.get_librknn_version()))
        if self.model_config_dict['RK_device_id'] is None:
            self._smart_record('device_id', bc.get_device_id()[0])
        else:
            self._smart_record('device_id', self.model_config_dict['RK_device_id'])
        freq_info = bc.scaling_freq()
        for _k, _s in freq_info.items():
            self._smart_record(_k, _s)
        return

    def _fill_the_report(self):
        from utils.shell_utils import check_file
        self._smart_record('framework', self.model_config_dict['model_framework'])
        self._smart_record('src_model', self.model_config_dict['load'].get('model', 'query_failed'))
        self._smart_record('src_model_md5', check_file(self.model_config_dict['load'].get('model', 'query_failed'), 'md5'))

        # input shape
        record_shape = {}
        for _in, _info in self.model_config_dict['inputs'].items():
            record_shape[_in] = _info['shape']
        self._smart_record('input_shape', record_shape)

        # output shape
        record_shape = self.model_config_dict.get('output_shape_by_run',['query_failed'])
        if record_shape[0]!='query_failed':
            _rs = {}
            for i in range(len(record_shape)):
                _rs["output_{}".format(i)] = record_shape[i]
            record_shape = _rs
        self._smart_record('output_shape', record_shape) 

        # model path
        if self.model_config_dict.get('pre_compile', 'off') == 'online' and \
            os.path.exists(self.model_config_dict['export_pre_compile_path']):
            _m_path = self.model_config_dict['export_pre_compile_path']
        else:
            _m_path = self.model_config_dict['export_rknn']['export_path']
        self._smart_record('rknn_model', _m_path)
        self._smart_record('rknn_model_md5', check_file(_m_path, 'md5'))

        # dtype
        if self.model_config_dict['build']['do_quantization']:
            self._smart_record('dtype', self.model_config_dict['config']['quantized_dtype'])
        else:
             self._smart_record('dtype', {1:'bfloat16', 2:'float16'}[self.model_config_dict['TOOLKIT_MAIN_VERSION']])
        return

    def _init_convinient_key_map(self):
        if not hasattr(self, "_report_key_map"):
            self._report_key_map = {}
            def _gen_convinient_key_map(_dict, _map, previous_level):
                for key, item in _dict.items():
                    _level = deepcopy(previous_level) + [key]
                    if isinstance(item, dict):
                        _gen_convinient_key_map(item, _map, _level)
                    else:
                        for i in range(len(_level)):
                            new_key = '.'.join(_level[-(i+1):])
                            if new_key in _map:
                                _map[new_key] = "INVALID_REPEAT"
                            else:
                                _map[new_key] = ''.join(["['{}']".format(_k) for _k in _level])
            
            _gen_convinient_key_map(self.report_info, self._report_key_map, [])


    def _smart_record(self, key, value):
        # self._init_convinient_key_map()
        assert key in self._report_key_map, "ERROR: {} not in the map".format(key)
        assert self._report_key_map[key]!= "INVALID_REPEAT", "ERROR: {} repeat in the map, please using more precision key".format(key)
        if isinstance(value, str):
            _command = "self.report_info{} = '{}'".format(self._report_key_map[key], value)
        else:
            _command = "self.report_info{} = {}".format(self._report_key_map[key], value)
        exec(_command)

    def extract_report(self, path, save_to_model_dir=False):
        self._fill_the_report()

        indent = 2
        _str = _dict_to_str(self.report_info, indent, new_line_between_dict=True)
        with open(path, 'w') as f:
            for _l in _str:
                f.write(_l)

        if save_to_model_dir:
            model_path = self.model_config_dict['export_rknn']['export_path']
            assert model_path.endswith('.rknn'), "ERROR: model path should end with .rknn"
            save_path = model_path[:-5] + '_@{}'.format(self.model_config_dict['RK_device_platform']) + '.yml'
            with open(save_path, 'w') as f:
                for _l in _str:
                    f.write(_l)