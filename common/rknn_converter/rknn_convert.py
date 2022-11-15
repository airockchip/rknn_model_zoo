import argparse
import os
import sys


# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]))

from config_parser import RKNN_config_container
from phase import convert_phase, validate_phase


def convert(model_config_dict, args):
    cp = convert_phase(model_config_dict, args)
    rknn = cp.convert()
    # rknn.accuracy_analysis('dataset.txt', target='rv1126')

    print('Convert Done!')
    # return
    vp = validate_phase(rknn, model_config_dict, args)

    if args.eval_perf:
        if model_config_dict['RK_device_id'] == 'simulator':
            print('Warning: Simulator does not support Evaluate Performance!')
        else:
            vp.Eval_perf(10, perf_debug=False)

    if args.eval_memory:
        if model_config_dict['RK_device_id'] == 'simulator':
            print('Warning: Simulator does not support Evaluate Memory!')
        else:
            vp.Eval_memory()

    if args.python_api_test:
        vp.Compare_convert_dist_via_python()

    if args.capi_test:
        vp.Compare_convert_dist_via_Capi(100)

    if args.capi_zero_copy_test:
        vp.Compare_convert_dist_via_Capi(100, 'zero_copy')

    # vp.Compare_rknn_python_Capi_dist(1)
    if args.report:
        vp.extract_report('report.yml')
    vp.release()
    
    return vp.report_info


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')
    # required params
    parser.add_argument('--yml_path', required=True, type=str, help='config file path')

    # optional params
    parser.add_argument('--capi_test', action="store_true", help='compute convert loss via capi, get run time')
    parser.add_argument('--capi_zero_copy_test', action="store_true", help='compute convert loss via capi zero_copy, get run time.[If input node is not quant, this may not work]')
    parser.add_argument('--eval_perf', action="store_true", help='eval performance via python api)')
    parser.add_argument('--eval_memory', action="store_true", help='eval memory via python api)')
    parser.add_argument('--output_path', required=False, default='AUTO', help='output rknn model path')
    parser.add_argument('--python_api_test', action="store_true", help='compute convert loss via python')
    parser.add_argument('--report', action="store_true", help='call to record time/result while validate')
    parser.add_argument('--overwrite', required=False, default='check', help="yes for always rewrite, no for always don't write, check for judge by user")

    args = parser.parse_args()

    parser = RKNN_config_container(args.yml_path)
    parser.parse()
    parser.update_path('export_path', args.output_path)
    
    parser.print_config()
    config_dict = parser.push_back()

    convert(config_dict, args)