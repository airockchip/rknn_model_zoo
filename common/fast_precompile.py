import os

PRE_SCRIPT = '/home/xz/Documents/git_rk/rknn-toolkit/examples/common_function_demos/export_rknn_precompile_model/export_rknn_precompile_model.py'
WEIGHT_ZOOM = '/home/xz/Documents/gitlab_model_zoo/weight_zoom'
PLATFORM = 'RV1109_1126'
# PLATFORM = 'RK1808_3399pro'


push_to_devices = False
rm_origin_model = False

Models = [
    'models/CV/object_detection/yolo/yolov5/deploy_models/toolkit1/model_cvt/{}/yolov5m_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),
    'models/CV/object_detection/yolo/yolov5/deploy_models/toolkit1/model_cvt/{}/yolov5s_relu_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),
    'models/CV/object_detection/yolo/yolov5/deploy_models/toolkit1/model_cvt/{}/yolov5s_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),

    'models/CV/object_detection/yolo/yolov7/deploy_models/toolkit1/model_cvt/{}/yolov7_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),
    'models/CV/object_detection/yolo/yolov7/deploy_models/toolkit1/model_cvt/{}/yolov7-tiny_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),

    'models/CV/object_detection/yolo/YOLOX/deploy_models/toolkit1/model_cvt/{}/yoloxm_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),
    'models/CV/object_detection/yolo/YOLOX/deploy_models/toolkit1/model_cvt/{}/yoloxs_tk1_{}_u8.rknn'.format(PLATFORM, PLATFORM),
]


for i,_m in enumerate(Models):
    print('{}/{}:'.format(i+1, len(Models)))
    new_path = _m.split('.rknn')[0]+'_precompile.rknn'
    commands = [
        'cd {}'.format(WEIGHT_ZOOM),
        'python {} {} {}'.format(PRE_SCRIPT, _m, new_path)
    ]
    if not os.path.exists(os.path.join(WEIGHT_ZOOM,new_path)):
        os.system('\n'.join(commands))

    # push to devices
    if push_to_devices:
        commands = [
            'cd {}'.format(WEIGHT_ZOOM),
            'adb push {} /userdata/rknn_yolo_demo/model/'.format(new_path)
        ]
        os.system('\n'.join(commands))

    # remove normal rknn
    if rm_origin_model:
        commands = [
            'cd {}'.format(WEIGHT_ZOOM),
            'rm {}'.format(_m)
        ]
        os.system('\n'.join(commands))