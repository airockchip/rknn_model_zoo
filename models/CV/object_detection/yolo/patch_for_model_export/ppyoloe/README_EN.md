## Export PP-YOLOE model with optimization for RKNN

### 1.PP-YOLOE discription

PP-YOLOE is an excellent single-stage anchor-free model based on PP-YOLOv2. For more detail, please refer [here](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.6/configs/ppyoloe/README.md).



### 2.Optimize for RKNN

For highest performance on rknpu, we change the output structure of the model. This will influence the post-process  logic.

- DFL block was moved into the post-process stage.
- A new branch added. Working as sum of class score, this branch could speedup the post-process stage in some case.



### 3.How to use

#### 3.1 Apply patch

```
git clone https://github.com/PaddlePaddle/PaddleDetection
cd PaddleDetection
git checkout release/2.5
git apply ../ppyoloe_rknn_optimize.patch
```

- This patch was valid on Version 2.5.



#### 3.2 Export onnx model

- save paddle model

```
python ./tools/export_model.py -c configs/ppyoloe/ppyoloe_plus_crn_s_80e_coco.yml -o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_plus_crn_s_80e_coco.pdparams exclude_nms=True trt=True exclude_post_process=True use_gpu=False --rknn
```

- convert paddle model to ONNX model

```
pip install paddle2onnx
paddle2onnx --model_dir output_inference/ppyoloe_plus_crn_s_80e_coco --model_filename model.pdmodel --params_filename model.pdiparams --opset_version 11 --save_file ppyoloe_plus_crn_s_80e_coco.onnx
```

