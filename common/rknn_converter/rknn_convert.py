import argparse
import os
import sys


# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]))

from config_parser import RKNN_config_container, get_example_img_from_dataset
from phase import convert_phase, validate_phase
from macro_define.rknpu import NPU_VERSION_1_DEVICES

def convert(model_config_dict, args):
    cp = convert_phase(model_config_dict, args)
    rknn = cp.convert()
    # for toolkit2
    # rknn.accuracy_analysis(get_example_img_from_dataset(model_config_dict['dataset'], False)[0],
    #                         target=model_config_dict['RK_device_platform'],
    #                         )

    # for toolkit1
    # rknn.accuracy_analysis('./dataset.txt',
    #                     target=model_config_dict['RK_device_platform'],
    #                     )
    # return

    if model_config_dict['RK_device_platform'].upper() in NPU_VERSION_1_DEVICES and \
        model_config_dict.get('pre_compile', 'off') != 'off':
        print('Convert Done! outpath: {}'.format(model_config_dict['export_pre_compile_path']))
    else:
        print('Convert Done! outpath: {}'.format(model_config_dict['export_rknn']['export_path']))
    vp = validate_phase(rknn, model_config_dict, args)

    if args.eval_perf:
        vp.eval_perf(20, perf_debug=False)

    if args.eval_memory:
        vp.eval_memory()

    if args.python_api_test:
        # for tt in range(50):
            vp.compare_convert_dist_via_python()

    if args.capi_test:
        # for tt in range(100):
            vp.compare_convert_dist_via_Capi(20)

    if args.capi_zero_copy_test:
        vp.compare_convert_dist_via_Capi(20, 'zero_copy')

    # vp.compare_rknn_python_Capi_dist(1)
    if args.report:
        vp.extract_report('report.yml', save_to_model_dir=True)
    vp.release()
    
    return vp.report_info


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')
    # required params
    parser.add_argument('--yml_path', required=True, type=str, help='config file path')

    # optional params
    parser.add_argument('--eval_perf', action="store_true", help='eval performance via python api)')
    parser.add_argument('--eval_memory', action="store_true", help='eval memory via python api)')
    parser.add_argument('--python_api_test', action="store_true", help='compute convert loss via python')
    parser.add_argument('--capi_test', action="store_true", help='compute convert loss via capi, get run time')
    parser.add_argument('--capi_zero_copy_test', action="store_true", help='compute convert loss via capi zero_copy, get run time.[If input node is not quant, this may not work]')
    
    parser.add_argument('--report', action="store_true", help='call to record time/result while validate')
    parser.add_argument('--eval_all', action="store_true", help='enable all test')
    
    parser.add_argument('--generate_random_input', action="store_true", help="try to generate input for model")
    parser.add_argument('--output_path', required=False, default='AUTO', help='output rknn model path')
    parser.add_argument('--overwrite', required=False, default='check', help="yes for always rewrite, no for always don't write, check for judge by user")
    parser.add_argument('--target_platform', required=False, default='AUTO', help='target platform')

    args = parser.parse_args()
    if args.eval_all:
        args.eval_perf = True
        args.eval_memory = True
        args.python_api_test = True
        args.capi_test = True
        args.capi_zero_copy_test = True
        args.report = True

    parser = RKNN_config_container(args.yml_path, args.target_platform)
    parser.parse(args.generate_random_input)
    parser.update_path('export_path', args.output_path)
    
    parser.print_config()
    config_dict = parser.push_back()

    convert(config_dict, args)