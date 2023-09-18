###
 # Copyright (c) 2022 by Rockchip Electronics Co., Ltd. All Rights Reserved.
 # 
 # @LastEditors: Zen.xing
 # @Description: Regenerate rknn model. Run this script if RKNN-Toolkit1/2 new version was release.
### 

from copy import deepcopy
import sys
import os

import yaml
def join_constructor(loader, node):
    seq = loader.construct_sequence(node)
    return ''.join([str(i) for i in seq])
yaml.add_constructor('!str_join', join_constructor)

realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
# sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')]))

from common.macro_define.rknpu import NPU_VERSION_1_DEVICES, NPU_VERSION_2_DEVICES, PLATFROM_COMPATIBAL_TRANSFORMER_MAP, PLATFROM_COMPATIBAL_TRANSFORMER_MAP_REVERSE, RKNN_DEVICES_ALL
from common.utils.shell_utils import *

# ==== config =====
SET_TOOLKIT = 2
CONVERTER_PATH="/home/xz/Documents/gitlab_model_zoo/rknn_model_zoo/common/rknn_converter/rknn_convert.py"
IGNORE_KEY = ['hyper_config']

PLATFORM_VALID = ['rv1106']
PLATFORM_VALID = [_d.upper() for _d in PLATFORM_VALID]

OVERWRITE = 'no'
EVAL_ALL_FUNC = True
CAPI_TEST = True
RK_DEVICE_ID = None

# ==== func ======
def flatten_list(_l):
    output_list = []
    for _ in _l:
        if isinstance(_, list):
            output_list.extend(flatten_list(_))
        else:
            output_list.append(_)
    return output_list

def _get_model_framework(path):
    if path.endswith('.onnx'):
        return 'onnx'
    elif path.endswith('.pt') or path.endswith('.torchscript'):
        return 'pytorch'
    else:
        assert False, "Not support model format"


# ==== main function ====
def main(task_path):
    with open(task_path, 'r') as f:
        task_config = yaml.load(f)

    task_len = len(task_config)
    for t, (_task, info) in enumerate(task_config.items()):
        if _task in IGNORE_KEY:
            continue

        print("\n{} Start {} task [{}/{}] {}".format('='*10, _task, t+1, task_len, '='*10))
        print("Exec_path: {}".format(info['exec_path']))
        print("Base_yml: {}".format(info['yml_path']))
        
        devices = flatten_list(info['platform'])
        devices = [_.upper() for _ in devices]
        if PLATFORM_VALID is not None:
            devices = [_ for _ in devices if _ in PLATFORM_VALID]
        if SET_TOOLKIT == 1:
            devices = [_ for _ in devices if _ in NPU_VERSION_1_DEVICES]
        elif SET_TOOLKIT == 2:
            devices = [_ for _ in devices if _ in NPU_VERSION_2_DEVICES]
        print("Devices: {}".format(devices))

        with open(os.path.join(info['exec_path'], info['yml_path']), 'r') as f:
            yml_config = yaml.safe_load(f)

        for _device in devices:
            for _m in info['model']:
            # reset yml
                print("    {} - {} ...".format(_device.upper(),_m), end='\r')
                yml_config['model_file_path'] = os.path.join(info['exec_path'], _m)
                yml_config['model_framework'] = _get_model_framework(_m)
                yml_config['RK_device_platform'] = _device
                yml_config['RK_device_id'] = RK_DEVICE_ID

                tmp_config = os.path.join(info['exec_path'], 'tmp_config.yml')
                with open(tmp_config, 'w') as f:
                    yaml.dump(yml_config, f)

                # start convert
                if EVAL_ALL_FUNC:
                    _run_cmd = "python {} --yml_path tmp_config.yml --overwrite {} --eval_all".format(CONVERTER_PATH, OVERWRITE),
                    # _run_cmd = "python {} --yml_path tmp_config.yml --overwrite {} --capi_test --report".format(CONVERTER_PATH, OVERWRITE),
                else:
                    _run_cmd = "python {} --yml_path tmp_config.yml --overwrite {}".format(CONVERTER_PATH, OVERWRITE),
                
                commands = [
                    "cd {}".format(os.path.join(info['exec_path'])),
                    _run_cmd[0],
                    # "python {} --yml_path tmp_config.yml --overwrite {}  > log.txt".format(CONVERTER_PATH, OVERWRITE),
                    "rm tmp_config.yml",
                    "rm log.txt"
                    ]
                convert_log = run_shell_command(commands)
                print("    {} - {} CONVERT FINISH".format(_device.upper(),_m))


                local_rknn_model = None
                rknn_name = None
                for _l in convert_log:
                    if _l.startswith("Convert Done! outpath: "):
                        local_rknn_model = _l.split(':')[-1].strip()
                        rknn_name = os.path.basename(local_rknn_model)
                        break
                local_rknn_model = os.path.join(info['exec_path'], local_rknn_model)


                if CAPI_TEST and 'capi_test' in info and local_rknn_model is not None:
                    ct_info = info['capi_test']
                    if not check_devices_available():
                        continue

                    for t_name, t_detail in ct_info.items():
                        t_detail = deepcopy(t_detail)

                        # 修正remote dir
                        if _device in ['RV1103', 'RV1106']:
                            t_detail['remote_dir'] = t_detail['remote_dir'].replace('/data/', '/tmp/')

                        if not check_file(t_detail['remote_dir'], 'exists', remote=True):
                            continue

                        remote_rknn_model = os.path.join(t_detail['remote_dir'], rknn_name)
                        if not check_file(remote_rknn_model, 'exists', remote=True):
                            run_shell_command(['adb push {} {}'.format(local_rknn_model, remote_rknn_model)])
                        
                        cmd = deepcopy(t_detail['cmd'])
                        for p, _k in enumerate(t_detail['cmd']):
                            if _k == 'RKNN':
                                cmd[p] = rknn_name

                        test_commands = [
                            'cd {}'.format(t_detail['remote_dir']),
                            'export LD_LIBRARY_PATH=./lib',
                            # '{} > log.txt'.format(' '.join(t_detail['cmd'])),
                            '{}'.format(' '.join(cmd)),
                        ]
                        capi_log = run_shell_command(test_commands, remote=True)

                        local_log_save_path = rknn_name.replace('.rknn', '_{}_log.txt'.format(t_name))
                        local_log_save_path = os.path.join(os.path.dirname(local_rknn_model), local_log_save_path)
                        with open(local_log_save_path, 'w') as f:
                            for _l in capi_log:
                                f.write(_l)
                        
                        for _file in t_detail['save_file']:
                            remote_path = os.path.join(t_detail['remote_dir'], _file)
                            local_file = _file.split('.')
                            local_file[0] = rknn_name.split('.')[0] + '_' + local_file[0] + '_' + t_name
                            local_file = '.'.join(local_file) 
                            local_path = os.path.join(os.path.dirname(local_rknn_model), local_file)
                            if check_file(remote_path, 'exists', remote=True):
                                run_shell_command(['adb pull {} {}'.format(remote_path, local_path)])
                                run_shell_command(['rm {}'.format(remote_path)], remote=True)
                            else:
                                print("    {} not found".format(remote_path))
                        run_shell_command(['rm {}'.format(remote_rknn_model)], remote=True)

                    print("    {} - {} CAPI TEST FINISH".format(_device.upper(),_m))

    print("done!")

if __name__ == '__main__':
    # test_file = 'yolo_v2.yml'
    test_file = 'yolo_v1.yml'
    main(test_file)

    # for _f in ['yolo_v1.yml', 'yolo_v2.yml']:
    #     main(_f)