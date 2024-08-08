[简体中文](README_CN.md) | [English](README.md)



# RKNN Model Zoo

## 简介

RKNN Model Zoo基于 RKNPU SDK 工具链开发, 提供了目前主流算法的部署例程. 例程包含导出RKNN模型, 使用 Python API, CAPI 推理 RKNN 模型的流程.

- 支持 `RK3562`, `RK3566`, `RK3568`, `RK3588`, `RK3576` 平台。
- 部分支持`RV1103`, `RV1106` 
- 支持 `RK1808`, `RV1109`, `RV1126` 平台。



## 依赖库安装

RKNN Model Zoo依赖 RKNN-Toolkit2 进行模型转换, 编译安卓demo时需要安卓编译工具链, 编译Linux demo时需要Linux编译工具链。这些依赖的安装请参考 https://github.com/airockchip/rknn-toolkit2/tree/master/doc 的 Quick Start 文档.

- 请注意, 安卓编译工具链建议使用 `r18` 或 `r19` 版本. 使用其他版本可能会遇到 Cdemo 编译失败的问题.



## 模型支持说明

以下demo除了从对应的仓库导出模型, 也可从网盘 https://console.zbox.filez.com/l/8ufwtG (提取码: rknn) 下载模型文件.

| Category | Name | Dtype | Model Download Link | Support platform |
| --- | --- | --- | --- | --- |
| 图像分类 | [mobilenet](https://github.com/onnx/models/tree/8e893eb39b131f6d3970be6ebd525327d3df34ea/vision/classification/mobilenet/model/mobilenetv2-12.onnx) | FP16/INT8 | [mobilenetv2-12.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mobilenet/mobilenetv2-12.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分类 | [resnet](https://github.com/onnx/models/tree/8e893eb39b131f6d3970be6ebd525327d3df34ea/vision/classification/resnet/model/resnet50-v2-7.onnx) | FP16/INT8 | [resnet50-v2-7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/resnet/resnet50-v2-7.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测     | [yolov5](https://github.com/airockchip/yolov5) | FP16/INT8 | [./yolov5s_relu.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s_relu.onnx)<br/>[./yolov5n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5n.onnx)<br/>[./yolov5s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s.onnx)<br/>[./yolov5m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5m.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov6](https://github.com/airockchip/yolov6) | FP16/INT8 | [./yolov6n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6n.onnx)<br/>[./yolov6s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6s.onnx)<br/>[./yolov6m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6m.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov7](https://github.com/airockchip/yolov7) | FP16/INT8 | [./yolov7-tiny.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7-tiny.onnx)<br/>[./yolov7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov8](https://github.com/airockchip/ultralytics_yolov8) | FP16/INT8 | [./yolov8n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8n.onnx)<br/>[./yolov8s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8s.onnx)<br/>[./yolov8m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8m.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov8_obb](https://github.com/airockchip/ultralytics_yolov8) | INT8 | [./yolov8n-obb.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_obb/yolov8n-obb.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov10](https://github.com/THU-MIG/yolov10) | FP16/INT8 | [./yolov10n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov10/yolov10n.onnx)<br/>[./yolov10s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov10/yolov10s.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolox](https://github.com/airockchip/YOLOX) | FP16/INT8 | [./yolox_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_s.onnx)<br/>[./yolox_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_m.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [ppyoloe](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.6/configs/ppyoloe) | FP16/INT8 | [./ppyoloe_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_s.onnx)<br/>[./ppyoloe_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_m.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolo_world](https://github.com/AILab-CVC/YOLO-World) | FP16/INT8 | [./yolo_world_v2s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo_world/yolo_world_v2s.onnx)<br/>[./clip_text.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo_world/clip_text.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/> |
| 人体关键点   | [yolov8_pose](https://github.com/airockchip/ultralytics_yolov8) | INT8 | [./yolov8n-pose.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_pose/yolov8n-pose.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/> |
| 图像分割 | deeplabv3 | FP16/INT8 | [./deeplab-v3-plus-mobilenet-v2.pb](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/deeplabv3/deeplab-v3-plus-mobilenet-v2.pb) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [yolov5_seg](https://github.com/airockchip/yolov5) | FP16/INT8 | [./yolov5n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5n-seg.onnx)<br/>[./yolov5s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5s-seg.onnx)<br/>[./yolov5m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5m-seg.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [yolov8_seg](https://github.com/airockchip/ultralytics_yolov8) | FP16/INT8 | [./yolov8n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8n-seg.onnx)<br/>[./yolov8s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8s-seg.onnx)<br/>[./yolov8m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8m-seg.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [ppseg](https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8) | FP16/INT8 | [pp_liteseg_cityscapes.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppseg/pp_liteseg_cityscapes.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 人脸关键点   | [RetinaFace](https://github.com/biubug6/Pytorch_Retinaface) | INT8 | [RetinaFace_mobile320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_mobile320.onnx)<br/>[RetinaFace_resnet50_320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_resnet50_320.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 车牌识别 | [LPRNet](https://github.com/sirius-ai/LPRNet_Pytorch/) | FP16/INT8 | [./lprnet.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/LPRNet/lprnet.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 文字检测     | [PPOCR-Det](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.7) | FP16/INT8 | [../ppocrv4_det.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_det.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 文字识别     | [PPOCR-Rec](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.7) | FP16 | [../ppocrv4_rec.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_rec.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 自然语言翻译 | [lite_transformer](https://github.com/airockchip/lite-transformer) | FP16 | [lite-transformer-encoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-encoder-16.onnx)<br/>[lite-transformer-decoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-decoder-16.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图文匹配 | [clip](https://huggingface.co/openai/clip-vit-base-patch32) | FP16 | [./clip_images.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/clip/clip_images.onnx)<br/>[./clip_text.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/clip/clip_text.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/> |
| 语音识别 | [whisper](https://github.com/openai/whisper) | FP16 | [whisper_encoder_base_20s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/whisper/whisper_encoder_base_20s.onnx)<br/>[whisper_decoder_base_20s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/whisper/whisper_decoder_base_20s.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576<br/> |
| 语音分类 | [yamnet](https://www.tensorflow.org/hub/tutorials/yamnet) | FP16 | [yamnet_3s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yamnet/yamnet_3s.onnx) | RK3566\|RK3568\|RK3588\|RK3562\|RK3576 |



## Model performance benchmark(FPS)

| demo             | model_name                          | inputs_shape&nbsp;&nbsp;&nbsp;&nbsp; | dtype | RK3566<br />RK3568 | RK3562         | RK3588<br />@single_core | RK3576<br />@single_core | RV1109 | RV1126 | RK1808 |
| ---------------- | ----------------------------------- | ------------------------------------ | ----- | ------------------ | -------------- | ------------------------ | ------------------------ | ------ | ------ | ------ |
| mobilenet        | mobilenetv2-12                      | [1, 3, 224, 224]                     | INT8  | 188.4              | 276.8          | 453.0                    | 475.1                    | 212.9  | 322.3  | 170.3  |
| resnet           | resnet50-v2-7                       | [1, 3, 224, 224]                     | INT8  | 38.5               | 52.5           | 109.0                    | 97.5                     | 24.4   | 36.2   | 37.1   |
| yolov5           | yolov5s_relu                        | [1, 3, 640, 640]                     | INT8  | 26.1               | 32.5           | 64.3                     | 65.5                     | 20.2   | 29.2   | 37.2   |
|                  | yolov5n                             | [1, 3, 640, 640]                     | INT8  | 39.9               | 47.1           | 76.5                     | 112.7                    | 36.3   | 53.2   | 61.2   |
|                  | yolov5s                             | [1, 3, 640, 640]                     | INT8  | 19.5               | 23.1           | 45.9                     | 56.5                     | 13.6   | 20.0   | 28.2   |
|                  | yolov5m                             | [1, 3, 640, 640]                     | INT8  | 8.8                | 10.5           | 20.3                     | 23.9                     | 5.8    | 8.5    | 13.3   |
| yolov6           | yolov6n                             | [1, 3, 640, 640]                     | INT8  | 48.3               | 55.3           | 102.0                    | 109.5                    | 37.8   | 56.8   | 66.8   |
|                  | yolov6s                             | [1, 3, 640, 640]                     | INT8  | 15.2               | 16.7           | 35.3                     | 34.0                     | 10.8   | 16.3   | 24.1   |
|                  | yolov6m                             | [1, 3, 640, 640]                     | INT8  | 7.3                | 8.2            | 17.5                     | 17.3                     | 5.6    | 8.3    | 11.5   |
| yolov7           | yolov7-tiny                         | [1, 3, 640, 640]                     | INT8  | 28.4               | 36.0           | 71.1                     | 75.3                     | 15.4   | 22.4   | 37.2   |
|                  | yolov7                              | [1, 3, 640, 640]                     | INT8  | 4.7                | 5.6            | 11.2                     | 12.7                     | 3.3    | 4.8    | 7.4    |
| yolov8           | yolov8n                             | [1, 3, 640, 640]                     | INT8  | 34.5               | 39.7           | 67.0                     | 89.5                     | 24.0   | 35.4   | 42.3   |
|                  | yolov8s                             | [1, 3, 640, 640]                     | INT8  | 15.4               | 17.6           | 36.0                     | 40.7                     | 8.9    | 13.1   | 19.1   |
|                  | yolov8m                             | [1, 3, 640, 640]                     | INT8  | 6.7                | 8.0            | 15.7                     | 16.5                     | 3.9    | 5.8    | 9.1    |
| yolov8_obb       | yolov8n-obb                         | [1, 3, 640, 640]                     | INT8  | 34.6               | 40.4           | 67.2                     | 91.4                     | 25.1   | 37.3   | 42.8   |
| yolov10          | yolov10n                            | [1, 3, 640, 640]                     | INT8  | 20.0               | 32.9           | 55.7                     | 80.6                     | /      | /      | /      |
|                  | yolov10s                            | [1, 3, 640, 640]                     | INT8  | 9.9                | 16.2           | 32.2                     | 39.6                     | /      | /      | /      |
| yolox            | yolox_s                             | [1, 3, 640, 640]                     | INT8  | 15.1               | 17.8           | 35.4                     | 42.2                     | 10.6   | 15.7   | 23.0   |
|                  | yolox_m                             | [1, 3, 640, 640]                     | INT8  | 6.7                | 7.9            | 15.5                     | 17.5                     | 4.6    | 6.8    | 10.7   |
| ppyoloe          | ppyoloe_s                           | [1, 3, 640, 640]                     | INT8  | 7.0                | 17.8           | 21.8                     | 43.6                     | 11.2   | 16.4   | 21.1   |
|                  | ppyoloe_m                           | [1, 3, 640, 640]                     | INT8  | 3.7                | 8.3            | 10.3                     | 18.1                     | 5.2    | 7.7    | 9.4    |
| yolo_world       | yolo_world_v2s                      | [1, 3, 640, 640]                     | INT8  | 5.8                | 10.8           | 16.7                     | 23.4                     | /      | /      | /      |
|                  | clip_text                           | [1, 20]                              | FP16  | 21.0               | 50.2           | 68.9                     | 54.7                     | /      | /      | /      |
| yolov8_pose      | yolov8n-pose                        | [1, 3, 640, 640]                     | INT8  | 20.9               | 29.5           | 48.9                     | 68.0                     | /      | /      | /      |
| deeplabv3        | deeplab-v3-plus-mobilenet-v2        | [1, 513, 513, 1]                     | INT8  | 12.0               | 19.9           | 34.4                     | 38.9                     | 10.1   | 13.0   | 4.4    |
| yolov5_seg       | yolov5n-seg                         | [1, 3, 640, 640]                     | INT8  | 31.7               | 38.0           | 63.5                     | 88.7                     | 28.6   | 42.2   | 49.6   |
|                  | yolov5s-seg                         | [1, 3, 640, 640]                     | INT8  | 14.9               | 17.6           | 35.1                     | 41.5                     | 9.6    | 14.0   | 22.5   |
|                  | yolov5m-seg                         | [1, 3, 640, 640]                     | INT8  | 6.8                | 8.1            | 15.9                     | 18.1                     | 4.7    | 6.8    | 10.8   |
| yolov8_seg       | yolov8n-seg                         | [1, 3, 640, 640]                     | INT8  | 27.8               | 32.1           | 55.1                     | 71.2                     | 18.6   | 27.6   | 32.9   |
|                  | yolov8s-seg                         | [1, 3, 640, 640]                     | INT8  | 11.9               | 13.6           | 27.5                     | 30.6                     | 6.6    | 9.8    | 14.6   |
|                  | yolov8m-seg                         | [1, 3, 640, 640]                     | INT8  | 5.2                | 6.1            | 12.2                     | 12.7                     | 3.1    | 4.6    | 6.9    |
| ppseg            | ppseg_lite_1024x512                 | [1, 3, 512, 512]                     | INT8  | 6.1                | 8.8            | 33.2                     | 29.5                     | 18.4   | 27.1   | 20.9   |
| RetinaFace       | RetinaFace_mobile320                | [1, 3, 320, 320]                     | INT8  | 117.7              | 175.7          | 168.7                    | 495.3                    | 144.8  | 212.5  | 198.5  |
|                  | RetinaFace_resnet50_320             | [1, 3, 320, 320]                     | INT8  | 17.9               | 24.4           | 45.1                     | 56.1                     | 14.6   | 20.8   | 24.6   |
| LPRNet           | lprnet                              | [1, 3, 24, 94]                       | INT8  | 47.3               | 144.2          | 234.7                    | 136.9                    | 30.6   | 47.6   | 30.1   |
| PPOCR-Det        | ppocrv4_det                         | [1, 3, 480, 480]                     | INT8  | 22.8               | 27.1           | 46.2                     | 64.5                     | 11.0   | 16.1   | 14.2   |
| PPOCR-Rec        | ppocrv4_rec                         | [1, 3, 48, 320]                      | FP16  | 19.3               | 50.8           | 63.2                     | 95.7                     | 1.0    | 1.6    | 6.7    |
| lite_transformer | lite-transformer-encoder-16         | embedding-256, token-16              | FP16  | 321.8              | 659.9          | 830.3                    | 804.2                    | 22.7   | 35.4   | 98.3   |
|                  | lite-transformer-decoder-16         | embedding-256, token-16              | FP16  | 121.2              | 216.4          | 312.5                    | 265.5                    | 48.0   | 65.8   | 109.9  |
| clip             | clip_images                         | [1, 3, 224, 224]                     | FP16  | 1.9                | 3.0            | 6.1                      | 6.3                      | /      | /      | /      |
|                  | clip_text                           | [1, 20]                              | FP16  | 21.4               | 50.3           | 68.3                     | 54.7                     | /      | /      | /      |
| whisper          | encoder+decoder+NPU-outside process | 20s audio                            | FP16  | RTF <br>1.263      | RTF <br/>0.392 | RTF <br/>0.215           | RTF <br/>0.222           | /      | /      | /      |
| yamnet           | yamnet_3s                           | 3s audio                             | FP16  | RTF <br>0.014      | RTF <br/>0.008 | RTF <br/>0.003           | RTF <br/>0.005           | /      | /      | /      |

- 该性能数据基于各平台的最大NPU频率进行测试
- 该性能数据指模型推理的耗时, 不包含前后处理的耗时
- `/`表示当前版本暂不支持



## Demo编译说明

对于 Linux 系统的开发板:

```sh
./build-linux.sh -t <target> -a <arch> -d <build_demo_name> [-b <build_type>] [-m]
    -t : target (rk356x/rk3588/rk3576/rv1106/rk1808/rv1126)
    -a : arch (aarch64/armhf)
    -d : demo name
    -b : build_type(Debug/Release)
    -m : enable address sanitizer, build_type need set to Debug
Note: 'rk356x' represents rk3562/rk3566/rk3568, 'rv1106' represents rv1103/rv1106, 'rv1126' represents rv1109/rv1126

# 以编译64位Linux RK3566的yolov5 demo为例:
./build-linux.sh -t rk356x -a aarch64 -d yolov5
```

对于 Android 系统的开发板:

```sh
# 对于 Android 系统的开发板, 首先需要根据实际情况, 设置安卓NDK编译工具的路径
export ANDROID_NDK_PATH=~/opts/ndk/android-ndk-r18b
./build-android.sh -t <target> -a <arch> -d <build_demo_name> [-b <build_type>] [-m]
    -t : target (rk356x/rk3588/rk3576)
    -a : arch (arm64-v8a/armeabi-v7a)
    -d : demo name
    -b : build_type (Debug/Release)
    -m : enable address sanitizer, build_type need set to Debug

# 以编译64位Android RK3566的yolov5 demo为例:
./build-android.sh -t rk356x -a arm64-v8a -d yolov5
```



## 版本说明

| 版本  | 说明                                                         |
| ----- | ------------------------------------------------------------ |
| 2.1.0 | 新例程添加, 包含 yolov8_pose, yolov8_obb, yolov10, yolo_world, clip, whisper, yamnet <br>部分模型暂不支持 `RK1808`, `RV1109`, `RV1126` 平台, 将在下个版本添加支持 |
| 2.0.0 | 新增所有示例`RK3576`平台的支持 <br />支持`RK1808`,  `RV1109`, `RV1126`平台 |
| 1.6.0 | 提供目标检测、图像分割、OCR、车牌识别等多个例程<br />支持`RK3562`, `RK3566`, `RK3568`, `RK3588`平台<br />部分支持`RV1103`, `RV1106`平台 |
| 1.5.0 | 提供Yolo检测模型的demo                                       |



## 环境依赖

RKNN Model Zoo 的例程基于当前最新的 RKNPU SDK 进行验证。若使用低版本的 RKNPU SDK 进行验证, 推理性能、推理结果可能会有差异。

| 版本  | RKNPU2 SDK | RKNPU1 SDK |
| ----- | ---------- | ---------- |
| 2.1.0 | >=2.1.0    | >=1.7.5    |
| 2.0.0 | >=2.0.0    | >=1.7.5    |
| 1.6.0 | >=1.6.0    | -          |
| 1.5.0 | >=1.5.0    | >=1.7.3    |



## RKNPU相关资料

- RKNPU2 SDK: https://github.com/airockchip/rknn-toolkit2
- RKNPU1 SDK: https://github.com/airockchip/rknn-toolkit



## 许可证

[Apache License 2.0](./LICENSE)

