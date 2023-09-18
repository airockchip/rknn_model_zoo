## 导出PP-YOLOE模型，针对RKNN优化

### 1.PP-YOLOE 简介

PP-YOLOE是基于PP-YOLOv2的卓越的单阶段Anchor-free模型，更多信息请参考这里 [here](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.6/configs/ppyoloe/README_cn.md).



### 2.针对RKNN优化

为了在 RKNPU 上获得更优的推理性能，我们调整了模型的输出结构，这些调整会影响到后处理的逻辑，主要包含以下内容

- DFL 结构被移至后处理
- 新增额外输出，该输出为所有类别分数的累加，用于加速后处理的候选框过滤逻辑



### 3.使用方式

#### 3.1 打补丁

```
git clone https://github.com/PaddlePaddle/PaddleDetection
cd PaddleDetection
git checkout release/2.5
git apply ../ppyoloe_rknn_optimize.patch
```

- 这个 patch 基于 release/2.5 版本可正常生效



#### 3.2 导出 ONNX 模型

- 保存 paddle 模型

```
python ./tools/export_model.py -c configs/ppyoloe/ppyoloe_plus_crn_s_80e_coco.yml -o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_plus_crn_s_80e_coco.pdparams exclude_nms=True trt=True exclude_post_process=True use_gpu=False --rknn
```

- 将 paddle 模型转为 ONNX 模型

```
pip install paddle2onnx
paddle2onnx --model_dir output_inference/ppyoloe_plus_crn_s_80e_coco --model_filename model.pdmodel --params_filename model.pdiparams --opset_version 11 --save_file ppyoloe_plus_crn_s_80e_coco.onnx
```



#### 3.3 ONNX 模型转为 RKNN 模型

- 请参考 [README.md](../../README.md) 的说明

