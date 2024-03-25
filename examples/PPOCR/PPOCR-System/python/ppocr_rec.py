# Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
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
import argparse
import cv2
import numpy as np
import utils.operators
from utils.rec_postprocess import CTCLabelDecode

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('rknn_model_zoo')+1]))

os.environ["FLAGS_allocator_strategy"] = 'auto_growth'

REC_INPUT_SHAPE = [48, 320] # h,w
CHARACTER_DICT_PATH= '../model/ppocr_keys_v1.txt'

PRE_PROCESS_CONFIG = [ 
        {
            'NormalizeImage': {
                'std': [1, 1, 1],
                'mean': [0, 0, 0],
                'scale': '1./255.',
                'order': 'hwc'
            }
        }
        ]

POSTPROCESS_CONFIG = {
        'CTCLabelDecode':{
            "character_dict_path": CHARACTER_DICT_PATH,
            "use_space_char": True
            }   
        }
class TextRecognizer:
    def __init__(self, args) -> None:
        self.model, self.framework = setup_model(args)
        self.preprocess_funct = []
        for item in PRE_PROCESS_CONFIG:
            for key in item:
                pclass = getattr(utils.operators, key)
                p = pclass(**item[key])
                self.preprocess_funct.append(p)

        self.ctc_postprocess = CTCLabelDecode(**POSTPROCESS_CONFIG['CTCLabelDecode'])

    def preprocess(self, img):
        for p in self.preprocess_funct:
            img = p(img)

        if self.framework == 'onnx':
            image_input = img['image']
            image_input = image_input.reshape(1, *image_input.shape)
            image_input = image_input.transpose(0, 3, 1, 2)
            img['image'] = image_input
        return img
    
    def run(self, imgs):
        outputs=[]
        for img in imgs:
            img = cv2.resize(img, (REC_INPUT_SHAPE[1], REC_INPUT_SHAPE[0]))
            model_input = self.preprocess({'image':img})
            output = self.model.run([model_input['image']])
            preds = output[0].astype(np.float32)
            output = self.ctc_postprocess(preds)
            outputs.append(output)
        return outputs

def setup_model(args):
    model_path = args.rec_model_path
    if model_path.endswith('.rknn'):
        platform = 'rknn'
        from py_utils.rknn_executor import RKNN_model_container 
        model = RKNN_model_container(model_path, args.target, args.device_id)
    elif model_path.endswith('onnx'):
        platform = 'onnx'
        from py_utils.onnx_executor import ONNX_model_container
        model = ONNX_model_container(model_path)
    else:
        assert False, "{} is not rknn/onnx model".format(model_path)
    print('Model-{} is {} model, starting val'.format(model_path, platform))
    return model, platform