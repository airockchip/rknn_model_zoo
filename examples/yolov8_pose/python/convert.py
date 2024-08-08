import sys,os
from rknn.api import RKNN

DATASET_PATH = '../../../datasets/COCO/coco_subset_20.txt'
DEFAULT_RKNN_PATH = '../model/yolov8_pose.rknn'
DEFAULT_QUANT = True

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]));
        print("       platform choose from [rk3562,rk3566,rk3576,rk3568,rk3588]")
        print("       dtype choose from [i8] for [rk3562,rk3566,rk3568,rk3576,rk3588]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'u8']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type in ['i8', 'u8']:
            do_quant = True
        else:
            do_quant = False

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = DEFAULT_RKNN_PATH

    return model_path, platform, do_quant, output_path

if __name__ == '__main__':
    model_path, platform, do_quant, output_path = parse_arg()

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')

    rknn.config(mean_values=[[0, 0, 0]], std_values=[
                    [255, 255, 255]], target_platform=platform)
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=model_path)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    if platform in ["rv1109","rv1126","rk1808"] :
        ret = rknn.build(do_quantization=do_quant, dataset=DATASET_PATH, auto_hybrid_quant=True)
    else:
        if do_quant:
            rknn.hybrid_quantization_step1(
                dataset=DATASET_PATH,
                proposal= False,
                custom_hybrid=[['/model.22/cv4.0/cv4.0.0/act/Mul_output_0','/model.22/Concat_6_output_0'],
                                ['/model.22/cv4.1/cv4.1.0/act/Mul_output_0','/model.22/Concat_6_output_0'],
                                ['/model.22/cv4.2/cv4.2.0/act/Mul_output_0','/model.22/Concat_6_output_0']]
            )

            model_name=os.path.basename(model_path).replace('.onnx','')
            rknn.hybrid_quantization_step2(
                model_input = model_name+".model",          # 表示第一步生成的模型文件
                data_input= model_name+".data",             # 表示第一步生成的配置文件
                model_quantization_cfg=model_name+".quantization.cfg"  # 表示第一步生成的量化配置文件
            )
        else:
            ret = rknn.build(do_quantization=do_quant, dataset=DATASET_PATH)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print("output_path:",output_path)
    print('done')
    # Release
    rknn.release()
