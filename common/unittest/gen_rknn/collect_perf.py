import os
import sys
import yaml
def join_constructor(loader, node):
    seq = loader.construct_sequence(node)
    return ''.join([str(i) for i in seq])
yaml.add_constructor('!str_join', join_constructor)
from collections import namedtuple

realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
# sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')]))
from common.macro_define.rknpu import NPU_VERSION_1_DEVICES, NPU_VERSION_2_DEVICES, PLATFROM_COMPATIBAL_TRANSFORMER_MAP, PLATFROM_COMPATIBAL_TRANSFORMER_MAP_REVERSE, RKNN_DEVICES_ALL

WEIGHT_ZOO_PATH = "/home/xz/Documents/gitlab_model_zoo/weight_zoom/models/CV/object_detection/yolo"
# MODEL_ORDER = ["yolov5-s", "yolov5-m", "yolov7-tiny", "yolov7", "yolox-s", "yolox-m", "yolov6-n", 
#                 "yolov6-s", "yolov6-m", "yolov8-n", "yolov8-s", "yolov8-m", "ppyoloe-s", "ppyoloe-m"]
MODEL_ORDER = ['yolov7-tiny','yolov8-n','yolov5-s','yolov6-n','yolox-s','ppyoloe-s','yolov8-s','yolov6-s','yolov5-m','yolox-m','ppyoloe-m','yolov6-m','yolov8-m','yolov7']
FREQ_ORDER = ['cpu','ddr','npu']

record = namedtuple('record', ['chipname', 'cpu', 'npu', 'ddr', 'src_model', 'dtype', 'run'])

def get_all_sub_folder(path):
    all_folder = []
    for root, dirs, files in os.walk(path):
        for dir in dirs:
            all_folder.append(os.path.join(root, dir))
    return all_folder


def collect_record(main_path):
    all_sub_folder = get_all_sub_folder(main_path)
    perf_list = []

    for sub_folder in all_sub_folder:
        if os.path.basename(sub_folder) != 'model_cvt':
            continue

        for platform_name in os.listdir(sub_folder):
            platform_dir = os.path.join(sub_folder, platform_name)
            
            cache_files = os.listdir(platform_dir)
            for _f in cache_files:
                f_type = _f.split('.')[-1]
                if f_type != 'yml':
                    continue

                with open(os.path.join(platform_dir, _f), 'r') as f:
                    yml_config = yaml.safe_load(f)

                chipname = yml_config['Board_info']['chipname']
                cpu = yml_config['Board_info']['CPU_freq']['query']
                ddr = yml_config['Board_info']['DDR_freq']['query']
                npu = yml_config['Board_info']['NPU_freq']['query']
                src_model = os.path.basename(yml_config['Model_info']['src_model'])
                dtype = yml_config['Model_info']['dtype']
                if chipname.upper() in ['RV1106', 'RV1103']:
                    run = yml_config['Model_info']['RKNN_api(zero_copy)']['time_cost(ms)']['run']
                else:
                    run = yml_config['Model_info']['RKNN_api(normal)']['time_cost(ms)']['run']
                if run in ['None','none']: run = 0
                perf_list.append(record(chipname=chipname, cpu=cpu, npu=npu, ddr=ddr, src_model=src_model, dtype=dtype, run=run))

    print("Collect {} record finish!".format(len(perf_list)))

    return perf_list


def merge_record(record_list):
    maping_key = {
        'yolov5s.pt' : "yolov5-s",
        'yolov5m.pt': "yolov5-m",
        'yolov7-tiny.pt' : "yolov7-tiny",
        'yolov7.pt': "yolov7",
        'yoloxs.pt' : "yolox-s",
        'yoloxm.pt' : "yolox-m",
    
        'yolov6n.onnx' : "yolov6-n",
        'yolov6s.onnx' : "yolov6-s",
        'yolov6m.onnx' : "yolov6-m",
        'yolov8n_rknnopt.torchscript' : "yolov8-n",
        'yolov8s_rknnopt.torchscript' : "yolov8-s",
        'yolov8m_rknnopt.torchscript' : "yolov8-m",
        'ppyoloe_s_ext_sum.onnx' : "ppyoloe-s",
        'ppyoloe_m_ext_sum.onnx' : "ppyoloe-m",
    }

    valid_chipname = []
    for r in record_list:
        if r.chipname != None and r.chipname!='None' and r.chipname not in valid_chipname:
            valid_chipname.append(r.chipname)

    chips_freq = []
    chips_freq = [[0]*len(FREQ_ORDER) for _ in range(len(valid_chipname))]
    for r in record_list:
        if r.chipname not in valid_chipname:
            continue
        chip_index = valid_chipname.index(r.chipname)
        for _d in FREQ_ORDER:
            _f = getattr(r, _d)
            chip_index = valid_chipname.index(r.chipname)
            freq_index = FREQ_ORDER.index(_d)
            chips_freq[chip_index][freq_index] = _f

    chips_perf = [[0]*len(MODEL_ORDER) for _ in range(len(valid_chipname))]
    for r in record_list:
        if r.chipname not in valid_chipname or \
            r.src_model not in maping_key.keys():
            continue
        chip_index = valid_chipname.index(r.chipname)
        model_index = MODEL_ORDER.index(maping_key[r.src_model])
        chips_perf[chip_index][model_index] = r.run

    return zip(valid_chipname, chips_freq, chips_perf)



def main():
    src_perf_list = collect_record(WEIGHT_ZOO_PATH)
    result = merge_record(src_perf_list)
    print("model," + ','.join(FREQ_ORDER) + ',' + ','.join(MODEL_ORDER))
    for i, (chip, freq, perf) in enumerate(result):
        perf = [1000/p if p!=0 else 0 for p in perf]
        print(chip + ',' + ','.join([str(f) for f in freq]) + ',' + ','.join(['%.2f'%p for p in perf]))



if __name__ == '__main__':
    main()