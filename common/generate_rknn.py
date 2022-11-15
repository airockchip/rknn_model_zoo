###
 # Copyright (c) 2022 by Rockchip Electronics Co., Ltd. All Rights Reserved.
 # 
 # @LastEditors: Zen.xing
 # @Description: Regenerate rknn model. Run this script if RKNN-Toolkit1/2 new version was release.
### 

import sys
import os
from ruamel import yaml

SET_TOOLKIT = 1
WEIGHT_ZOOM_PATH="/home/xz/Documents/gitlab_model_zoo/weight_zoom"
CONVERTER_PATH="/home/xz/Documents/gitlab_model_zoo/rknn_model_zoo/common/rknn_converter/rknn_convert.py"
OVERWRITE = 'no'

platform_toolkit_map = {
    'rk1808': 1,
    'rk3399pro': 1,
    'rv1109': 1,
    'rv1126': 1,

    'rk3566': 2,
    'rk3568': 2,
    'rk3588': 2,
    'rv1103': 2,
    'rv1106': 2,
}


Task = {
    'yolov5_tk1':{
        'exec_path': "./models/CV/object_detection/yolo/yolov5/deploy_models/toolkit1",
        'yml_path': "../yolov5.yml",
        'model': ["./yolov5m_tk1.pt", 
                  "./yolov5s_tk1.pt",
                  "./yolov5s_relu_tk1.pt"],
        'platform': ['rk1808', 'rv1126']
    },

    'yolov5_tk2':{
        'exec_path': "./models/CV/object_detection/yolo/yolov5/deploy_models/toolkit2",
        'yml_path': "../yolov5.yml",
        'model': ["./yolov5m_tk2.pt", 
                  "./yolov5s_tk2.pt",
                  "./yolov5s_relu_tk2.pt"],
        'platform': ['rk3568', 'rk3588']
    },

    'yolov7_tk1':{
        'exec_path': "./models/CV/object_detection/yolo/yolov7/deploy_models/toolkit1",
        'yml_path': "../yolov7.yml",
        'model': ["./yolov7_tk1.pt", 
                  "./yolov7-tiny_tk1.pt"],
        'platform': ['rk1808', 'rv1126']
    },

    'yolov7_tk2':{
        'exec_path': "./models/CV/object_detection/yolo/yolov7/deploy_models/toolkit2",
        'yml_path': "../yolov7.yml",
        'model': ["./yolov7_tk2.pt", 
                  "./yolov7-tiny_tk2.pt"],
        'platform': ['rk3568','rk3588']
    },

    'yolox_tk1':{
        'exec_path': "./models/CV/object_detection/yolo/YOLOX/deploy_models/toolkit1",
        'yml_path': "../yolox.yml",
        'model': ["./yoloxm_tk1.pt", 
                  "./yoloxs_tk1.pt"],
        'platform': ['rk1808', 'rv1126']
    },

    'yolox_tk2':{
        'exec_path': "./models/CV/object_detection/yolo/YOLOX/deploy_models/toolkit2",
        'yml_path': "../yolox.yml",
        'model': ["./yoloxm_tk2.pt", 
                  "./yoloxs_tk2.pt"],
        'platform': ['rk3568', 'rk3588']
    },
}


def main():
    print("Now toolkit is set as {}".format(SET_TOOLKIT))
    print("Overwrite setting is {}".format(OVERWRITE))
    print("Convert for {}\n".format(Task.keys()))
    for i, (_task, info) in enumerate(Task.items()):
        print("{}/{} - {}:".format(i+1, len(Task), _task))
        for _device in info['platform']:
            if platform_toolkit_map[_device.lower()] != SET_TOOLKIT:
                # ignore unmatch platform
                print("    Toolkit-{} ignore {}".format(SET_TOOLKIT, _device))
                continue
            
            yml_path = os.path.join(WEIGHT_ZOOM_PATH, info['exec_path'], info['yml_path'])
            with open(yml_path, 'r') as f:
                yml_config = yaml.safe_load(f)

            for _m in info['model']:
                # reset yml
                print("    {} - {} ...".format(_device.upper(),_m), end='\r')
                yml_config['model_file_path'] = os.path.join(WEIGHT_ZOOM_PATH, info['exec_path'], _m)
                yml_config['RK_device_platform'] = _device

                tmp_config = os.path.join(WEIGHT_ZOOM_PATH, info['exec_path'], 'tmp_config.yml')
                with open(tmp_config, 'w') as f:
                    yaml.dump(yml_config, f)

                # start convert
                commands = [
                    "cd {}".format(os.path.join(WEIGHT_ZOOM_PATH, info['exec_path'])),
                    "python {} --yml_path tmp_config.yml --overwrite {}".format(CONVERTER_PATH, OVERWRITE),
                    # "python {} --yml_path tmp_config.yml --overwrite {}  > log.txt".format(CONVERTER_PATH, OVERWRITE),
                    "rm tmp_config.yml",
                    "rm log.txt",
                ]
                os.system('\n'.join(commands))
                print("    {} - {} FINISH".format(_device.upper(),_m))

    print("done!")

if __name__ == '__main__':
    main()