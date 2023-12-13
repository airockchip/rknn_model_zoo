# Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import os
import sys
from tabnanny import verbose
from rknn.api import RKNN


DATASET_PATH = '../../../../datasets/PPOCR/imgs/dataset_20.txt'
DEFAULT_RKNN_PATH = '../model/ppocrv4_det.rknn'
DEFAULT_QUANT = True

def parse_arg():
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]));
        print("       platform choose from [rk3562,rk3566,rk3568,rk3588]")
        print("       dtype choose from    [i8, fp]")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]

    do_quant = DEFAULT_QUANT
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type not in ['i8', 'fp']:
            print("ERROR: Invalid model type: {}".format(model_type))
            exit(1)
        elif model_type == 'i8':
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

    model = RKNN(verbose=False)

    # Config
    model.config(
        mean_values=[123.675, 116.28, 103.53],
        std_values=[58.395, 57.12, 57.375],
        target_platform=platform,
    )

    # Load ONNX model
    ret = model.load_onnx(model=model_path)

    # Build model
    ret = model.build(
        do_quantization=do_quant,
        dataset=DATASET_PATH)
    assert ret == 0, "Build model failed!"

    # Init Runtime
    # ret = model.init_runtime()
    # assert ret == 0, "Init runtime environment failed!"

    # Export
    if not os.path.exists(os.path.dirname(output_path)):
        os.mkdir(os.path.dirname(output_path))

    ret = model.export_rknn(
        output_path)
    assert ret == 0, "Export rknn model failed!"
    print("Export OK!")
