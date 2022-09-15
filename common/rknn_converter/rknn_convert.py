import argparse
import os
import sys

import numpy as np
from rknn.api import RKNN

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]))

from config_parser import RKNN_config_container


def compute_cos_dist(x,y):
    cos_dist= (x* y)/(np.linalg.norm(x)*(np.linalg.norm(y)))
    return cos_dist.sum()


def get_framework_excute_info(model_config_dict):
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
            framework_excute_info['mean_values'] = input_info['mean_values']
            framework_excute_info['std_values'] = input_info['std_values']

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

    return framework_excute_info


def convert(model_config_dict, args):
    overwrite = True
    if os.path.exists(model_config_dict['export_rknn']['export_path']):
        print('{} is already exists! Do you want to overwrite it? [Y/N]'.format(model_config_dict['export_rknn']['export_path']))
        key_in = ' '
        while key_in not in ['Y', 'N']:
            key_in = input("Y or N:")
        if key_in == 'N':
            overwrite = False

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
    elif overwrite is False:
        rknn = RKNN()
        rknn.load_rknn(model_config_dict['export_rknn']['export_path'])

    # Init runtime & export pre_compile_model
    already_init = False
    if model_config_dict['pre_compile'] == 'online':
        print('---> Export pre-complie model')
        if model_config_dict['RK_device_platform'] in ['RK3588','RK3568','RK3566']:
            print("  {} model needn't pre_compile. Ignore!".format(model_config_dict['RK_device_platform']))
        elif model_config_dict['RK_device_id'] == 'simulator':
            print("  simulator does not support exporting pre_compiled model. Ignore!")
        else:
            print('---> Init_rknn_runtime')
            rknn.init_runtime(target=model_config_dict['RK_device_platform'], device_id = model_config_dict['RK_device_id'], rknn2precompile=True)
            rknn.export_rknn_precompile_model(model_config_dict['export_pre_compile_path'])
            already_init = True

    if (args.compute_convert_loss or args.eval_perf) and \
       already_init is False:
        print('---> Init_rknn_runtime')
        if model_config_dict['RK_device_id'] != 'simulator':
            rknn.init_runtime(target=model_config_dict['RK_device_platform'], device_id = model_config_dict['RK_device_id'])
        else:
            rknn.init_runtime()

    # compute convert loss
    if args.compute_convert_loss and model_config_dict['input_example'] is not None:
        print('---> start compute convert loss')
        print('  compute rknn result')
        from image_utils.img_preprocesser_tools import Image_preprocessor
        rk_input_list = []
        for _index, input_info in model_config_dict['inputs'].items():
            imp = Image_preprocessor(input_info['input_example'], 'RGB')    # rknn_toolkit_1 always get RGB in.
            imp.resize(input_info['img_size'])
            rk_input_list.append(imp.get_input('rknn'))
        rknn_result = rknn.inference(rk_input_list)

        framework = model_config_dict['model_framework']
        print('  compute {} result'.format(framework))
        from framework_excuter.excuter import Excuter
        framework_excute_info = get_framework_excute_info(model_config_dict)
        model_runer = Excuter(framework_excute_info)
        framework_input_list = []
        for _index, input_info in model_config_dict['inputs'].items():
            imp = Image_preprocessor(input_info['input_example'], input_info['img_type'])
            imp.resize(input_info['img_size'])
            if framework not in ['caffe','darknet']:
                imp.normalize(input_info['mean_values'], input_info['std_values'])
            framework_input_list.append(imp.get_input(framework))
        framework_result = model_runer.inference(framework_input_list)
        model_type = 'fp'
        if model_config_dict['build']['do_quantization']:
            model_type = model_config_dict['config']['quantized_dtype']

        for i in range(len(framework_result)):
            cos_dist = compute_cos_dist(rknn_result[i], framework_result[i])
            print('     For output-{} : rknn({}) VS {} cos_similarity[{}]'.format(i, model_type, framework, cos_dist))
    elif args.compute_convert_loss and model_config_dict['input_example'] is None:
        print('WARNING: None example input exists, ignore compute convert loss')

    if args.eval_perf:
        print('---> Eval performance')
        rknn.eval_perf()

    # quantitative accuracy analysis in simulator
    ret = rknn.accuracy_analysis(inputs=['/mnt/hgfs/virtualmachineshare/rknn_model_zoo/datasets/fire/fire_00007.jpg'])
    if ret != 0:
        print('Accuracy analysis failed!') 

    rknn.release()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')
    # required params
    parser.add_argument('--yml_path', required=True, type=str, help='default')

    # optional params
    parser.add_argument('--output_path', required=False, default='model.rknn', help='output rknn.path')
    parser.add_argument('--compute_convert_loss', action="store_true", help='compute convert accuracy loss with cosine dist')
    parser.add_argument('--eval_perf', action="store_true", help='eval performance')

    args = parser.parse_args()

    parser = RKNN_config_container(args.yml_path)
    parser.parse()
    parser.update_path('export_path', args.output_path)
    
    parser.print_config()
    config_dict = parser.push_back()

    convert(config_dict, args)