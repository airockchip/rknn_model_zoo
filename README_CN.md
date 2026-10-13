[简体中文](README_CN.md) | [English](README.md)



# RKNN Model Zoo

## 简介

RKNN Model Zoo基于 RKNPU SDK 工具链开发, 提供了目前主流算法的部署例程. 例程包含导出RKNN模型, 使用 Python API, CAPI 推理 RKNN 模型的流程.

- 支持 `RK3562`, `RK3566`, `RK3568`, `RK3576`, `RK3588`, `RV1126B` 平台。
- 部分支持`RV1103`, `RV1106` 
- 支持  `RV1109`, `RV1126`, `RK1808` 平台。



## 依赖库安装

RKNN Model Zoo依赖 RKNN-Toolkit2 进行模型转换, 编译安卓demo时需要安卓编译工具链, 编译Linux demo时需要Linux编译工具链。这些依赖的安装请参考 https://github.com/airockchip/rknn-toolkit2/tree/master/doc 的 Quick Start 文档.

- 请注意, 安卓编译工具链建议使用 `r18` 或 `r19` 版本. 使用其他版本可能会遇到 Cdemo 编译失败的问题.
- 请注意, Linux编译工具链建议使用`gcc-linaro-6.3.1(aarch64)/gcc-arm-8.3(armhf)/armhf-uclibcgnueabihf(armhf, 用于RV1106/RV1103系列)`，使用其他版本可能会遇到Cdemo编译失败的问题。详细编译指南请参考 [Compilation_Environment_Setup_Guide_CN.md](./docs/Compilation_Environment_Setup_Guide_CN.md)



## 模型支持说明

以下demo除了从对应的仓库导出模型, 也可从网盘 https://console.zbox.filez.com/l/8ufwtG (提取码: rknn) 下载模型文件.

| Category | Name | Dtype | Model Download Link | Support platform |
| --- | --- | --- | --- | --- |
| 图像分类 | [mobilenet](https://github.com/onnx/models/tree/8e893eb39b131f6d3970be6ebd525327d3df34ea/vision/classification/mobilenet/model/mobilenetv2-12.onnx) | FP16/INT8 | [mobilenetv2-12.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mobilenet/mobilenetv2-12.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分类 | [resnet](https://github.com/onnx/models/tree/8e893eb39b131f6d3970be6ebd525327d3df34ea/vision/classification/resnet/model/resnet50-v2-7.onnx) | FP16/INT8 | [resnet50-v2-7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/resnet/resnet50-v2-7.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测     | [yolov5](https://github.com/airockchip/yolov5) | FP16/INT8 | [./yolov5s_relu.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s_relu.onnx)<br/>[./yolov5n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5n.onnx)<br/>[./yolov5s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s.onnx)<br/>[./yolov5m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov6](https://github.com/airockchip/yolov6) | FP16/INT8 | [./yolov6n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6n.onnx)<br/>[./yolov6s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6s.onnx)<br/>[./yolov6m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov7](https://github.com/airockchip/yolov7) | FP16/INT8 | [./yolov7-tiny.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7-tiny.onnx)<br/>[./yolov7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov8](https://github.com/airockchip/ultralytics_yolov8) | FP16/INT8 | [./yolov8n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8n.onnx)<br/>[./yolov8s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8s.onnx)<br/>[./yolov8m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov8_obb](https://github.com/airockchip/ultralytics_yolov8) | INT8 | [./yolov8n-obb.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_obb/yolov8n-obb.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolov10](https://github.com/THU-MIG/yolov10) | FP16/INT8 | [./yolov10n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov10/yolov10n.onnx)<br/>[./yolov10s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov10/yolov10s.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolo11](https://github.com/airockchip/ultralytics_yolo11) | FP16/INT8 | [./yolo11n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo11/yolo11n.onnx)<br/>[./yolo11s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo11/yolo11s.onnx)<br/>[./yolo11m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo11/yolo11m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolo26](https://github.com/ultralytics/ultralytics?tab=readme-ov-file) | FP16/INT8 | [./yolo26n.onnx](https://meta.zbox.filez.com/v/link/view/dddfdaa76107404a8a358673fa31a041)<br/>[./yolo26s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo26/yolo26s.onnx)<br/>[./yolo26m.onnx](https://meta.zbox.filez.com/v/link/view/198e90ee6bc94c71ad2002d5f9897ac4) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106|
| 物体检测 | [yolox](https://github.com/airockchip/YOLOX) | FP16/INT8 | [./yolox_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_s.onnx)<br/>[./yolox_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [ppyoloe](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.6/configs/ppyoloe) | FP16/INT8 | [./ppyoloe_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_s.onnx)<br/>[./ppyoloe_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_m.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 物体检测 | [yolo_world](https://github.com/AILab-CVC/YOLO-World) | FP16/INT8 | [./yolo_world_v2s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo_world/yolo_world_v2s.onnx)<br/>[./clip_text.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo_world/clip_text.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/> |
| 人体关键点   | [yolov8_pose](https://github.com/airockchip/ultralytics_yolov8) | INT8 | [./yolov8n-pose.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_pose/yolov8n-pose.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B |
| 图像分割 | deeplabv3 | FP16/INT8 | [./deeplab-v3-plus-mobilenet-v2.pb](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/deeplabv3/deeplab-v3-plus-mobilenet-v2.pb) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [yolov5_seg](https://github.com/airockchip/yolov5) | FP16/INT8 | [./yolov5n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5n-seg.onnx)<br/>[./yolov5s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5s-seg.onnx)<br/>[./yolov5m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5m-seg.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [yolov8_seg](https://github.com/airockchip/ultralytics_yolov8) | FP16/INT8 | [./yolov8n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8n-seg.onnx)<br/>[./yolov8s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8s-seg.onnx)<br/>[./yolov8m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8m-seg.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [ppseg](https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8) | FP16/INT8 | [pp_liteseg_cityscapes.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppseg/pp_liteseg_cityscapes.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图像分割 | [mobilesam](https://github.com/ChaoningZhang/MobileSAM) | FP16 | [mobilesam_encoder_tiny.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mobilesam/mobilesam_encoder_tiny.onnx)<br />[mobilesam_decoder.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mobilesam/mobilesam_decoder.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B |
| 人脸关键点   | [RetinaFace](https://github.com/biubug6/Pytorch_Retinaface) | INT8 | [RetinaFace_mobile320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_mobile320.onnx)<br/>[RetinaFace_resnet50_320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_resnet50_320.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 车牌识别 | [LPRNet](https://github.com/sirius-ai/LPRNet_Pytorch/) | FP16/INT8 | [./lprnet.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/LPRNet/lprnet.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RV1103\|RV1106<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 文字检测     | [PPOCR-Det](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.7) | FP16/INT8 | [../ppocrv4_det.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_det.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 文字识别     | [PPOCR-Rec](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.7) | FP16 | [../ppocrv4_rec.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_rec.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 自然语言翻译 | [lite_transformer](https://github.com/airockchip/lite-transformer) | FP16 | [lite-transformer-encoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-encoder-16.onnx)<br/>[lite-transformer-decoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-decoder-16.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/>RK1808\|RK3399PRO<br/>RV1109\|RV1126 |
| 图文匹配 | [clip](https://huggingface.co/openai/clip-vit-base-patch32) | FP16 | [./clip_images.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/clip/clip_images.onnx)<br/>[./clip_text.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/clip/clip_text.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/> |
| 语音识别 | [wav2vec2](https://github.com/facebookresearch/fairseq/tree/main/examples/wav2vec#wav2vec-20) | FP16 | [wav2vec2_base_960h_20s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/wav2vec2/wav2vec2_base_960h_20s.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B |
| 语音识别 | [whisper](https://github.com/openai/whisper) | FP16 | [whisper_encoder_base_20s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/whisper/whisper_encoder_base_20s.onnx)<br/>[whisper_decoder_base_20s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/whisper/whisper_decoder_base_20s.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/> |
| 语音识别 | [zipformer](https://huggingface.co/csukuangfj/k2fsa-zipformer-bilingual-zh-en-t) | FP16 | [encoder-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/encoder-epoch-99-avg-1.onnx)<br/>[decoder-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/decoder-epoch-99-avg-1.onnx)<br/>[joiner-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/joiner-epoch-99-avg-1.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/> |
| 语音分类 | [yamnet](https://www.tensorflow.org/hub/tutorials/yamnet) | FP16 | [yamnet_3s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yamnet/yamnet_3s.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B |
| 文字转语音 | [mms_tts](https://huggingface.co/facebook/mms-tts-eng) | FP16 | [mms_tts_eng_encoder_200.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mms_tts/mms_tts_eng_encoder_200.onnx)<br/>[mms_tts_eng_decoder_200.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mms_tts/mms_tts_eng_decoder_200.onnx) | RK3562\|RK3566\|RK3568\|RK3576\|RK3588\|RV1126B<br/> |



## Model performance benchmark(FPS)

| demo             | model_name                          | inputs_shape&nbsp;&nbsp;&nbsp;&nbsp; | dtype | RK3566<br />RK3568 | RK3562         | RK3588<br />@single_core | RK3576<br />@single_core | RV1126B | RV1109     | RV1126     | RK1808     |
| ---------------- | ----------------------------------- | ------------------------------------ | ----- | ------------------ | -------------- | ------------------------ | ------------------------ | ---------- | ---------- | ---------- | ---------------- |
| mobilenet        | mobilenetv2-12                      | [1, 3, 224, 224]        | INT8     | 192.2                 | 289.2   | 458.8       | 459.0     | 374.0 | 212.9      | 322.3      | 170.3      |
| resnet           | resnet50-v2-7                       | [1, 3, 224, 224]        | INT8     | 38.6                  | 55.0      | 112.5          | 96.0      | 78.2 | 24.4       | 36.2       | 37.1       |
| yolov5           | yolov5s_relu                        | [1, 3, 640, 640]        | INT8     | 27.1               | 33.7      | 66.5          | 65.1   | 55.6 | 20.2       | 29.2       | 37.2       |
|                  | yolov5n                             | [1, 3, 640, 640]        | INT8     | 41.7                  | 48.8      | 84.6           | 123.0     | 105.8 | 36.3       | 53.2       | 61.2       |
|                  | yolov5s                             | [1, 3, 640, 640]        | INT8     | 20.1                  | 23.8      | 49.1           | 59.6      | 51.2 | 13.6       | 20.0       | 28.2       |
|                  | yolov5m                             | [1, 3, 640, 640]        | INT8     | 8.7                   | 10.8      | 20.9           | 24.1      | 20.9 | 5.8        | 8.5        | 13.3       |
| yolov6           | yolov6n                             | [1, 3, 640, 640]        | INT8     | 49.9                  | 57.3      | 108.8          | 109.9     | 97.9 | 37.8       | 56.8       | 66.8       |
|                  | yolov6s                             | [1, 3, 640, 640]        | INT8     | 15.2                  | 17.2      | 36.7           | 33.8      | 30.2 | 10.8       | 16.3       | 24.1       |
|                  | yolov6m                             | [1, 3, 640, 640]        | INT8     | 7.3                   | 8.5       | 17.8           | 16.8      | 14.9 | 5.6        | 8.3        | 11.5       |
| yolov7           | yolov7-tiny                         | [1, 3, 640, 640]        | INT8     | 29.8                  | 37.4      | 73.6           | 78.7      | 66.9 | 15.4       | 22.4       | 37.2       |
|                  | yolov7                              | [1, 3, 640, 640]        | INT8     | 4.7                   | 5.9       | 11.9           | 12.8      | 11.4 | 3.3        | 4.8        | 7.4        |
| yolov8           | yolov8n                             | [1, 3, 640, 640]        | INT8     | 35.1                  | 41.7      | 76.3           | 97.7      | 83.0 | 24.0       | 35.4       | 42.3       |
|                  | yolov8s                             | [1, 3, 640, 640]        | INT8     | 15.4                  | 18.5      | 38.6           | 42.1      | 36.2 | 8.9        | 13.1       | 19.1       |
|                  | yolov8m                             | [1, 3, 640, 640]        | INT8     | 6.6                   | 8.2       | 16.3           | 16.5      | 14.7 | 3.9        | 5.8        | 9.1        |
| yolov8_obb       | yolov8n-obb                         | [1, 3, 640, 640]        | INT8     | 35.5                  | 42.1      | 77.1           | 100.7      | 85.0 | 25.1       | 37.3       | 42.8       |
| yolov10          | yolov10n                            | [1, 3, 640, 640]        | INT8     | 22.5                  | 34.7      | 63.9           | 88.7      | 74.0 | /          | /          | /          |
|                  | yolov10s                            | [1, 3, 640, 640]        | INT8     | 10.9                  | 17.0      | 34.8           | 41.5      | 35.0 | /          | /          | /          |
| yolo11           | yolo11n                             | [1, 3, 640, 640]        | INT8     | 22.6                  | 35.2      | 64.0           | 87.5      | 73.7 | 11.7       | 17.0       | 17.6       |
|                  | yolo11s                             | [1, 3, 640, 640]        | INT8     | 10.8                  | 16.8      | 34.6           | 40.7      | 34.2 | 5.0        | 7.3        | 8.4        |
|                  | yolo11m                             | [1, 3, 640, 640]        | INT8     | 4.8                   | 6.5       | 14.6           | 15.9      | 13.0 | 2.8        | 4.0        | 5.1        |
| yolo26 | yolo26n | [1, 3, 640, 640] | INT8 | 16.7 | 31.0 | 55.0 | 76.5 | 63.2 | / | / | / |
|  | yolo26s | [1, 3, 640, 640] | INT8 | 8.1 | 15.0 | 30.5 | 36.3 | 30.2 | / | / | / |
|  | yolo26m | [1, 3, 640, 640] | INT8 | 4.2 | 6.3 | 14.0 | 14.4 | 12.5 | / | / | / |
| yolox            | yolox_s                             | [1, 3, 640, 640]        | INT8     | 15.6                  | 18.3      | 37.6           | 42.8      | 37.2 | 10.6       | 15.7       | 23.0       |
|                  | yolox_m                             | [1, 3, 640, 640]        | INT8     | 6.7                   | 8.1       | 15.9           | 17.3      | 15.3 | 4.6        | 6.8        | 10.7       |
| ppyoloe          | ppyoloe_s                           | [1, 3, 640, 640]        | INT8     | 17.6                   | 21.6      | 39.7           | 49.0      | 41.5 | 11.2       | 16.4       | 21.1       |
|                  | ppyoloe_m                           | [1, 3, 640, 640]        | INT8     | 7.9                   | 9.7       | 18.2           | 19.1      | 16.8 | 5.2        | 7.7        | 9.4        |
| yolo_world       | yolo_world_v2s                      | [1, 3, 640, 640]        | INT8     | 5.9                 | 9.3      | 20.6        | 20.4      | 19.2 | /          | /          | /          |
|                  | clip_text                           | [1, 20]                 | FP16     | 30.5                  | 71.1   | 95.9        | 64.0      | 61.2 | /          | /          | /          |
| yolov8_pose      | yolov8n-pose                        | [1, 3, 640, 640]        | INT8     | 23.7                  | 32.4      | 58.3           | 75.0      | 59.1 | /          | /          | /          |
| deeplabv3        | deeplab-v3-plus-mobilenet-v2        | [1, 513, 513, 1]        | INT8     | 11.7                  | 21.0      | 34.7           | 37.8      | 30.9 | 10.1       | 13.0       | 4.4        |
| yolov5_seg       | yolov5n-seg                         | [1, 3, 640, 640]        | INT8     | 34.4                  | 40.3      | 71.5           | 96.4      | 83.8 | 28.6       | 42.2       | 49.6       |
|                  | yolov5s-seg                         | [1, 3, 640, 640]        | INT8     | 15.5                  | 18.3      | 37.4           | 43.2      | 37.5 | 9.6        | 14.0       | 22.5       |
|                  | yolov5m-seg                         | [1, 3, 640, 640]        | INT8     | 6.8                   | 8.4       | 16.4           | 17.8      | 15.7 | 4.7        | 6.8        | 10.8       |
| yolov8_seg       | yolov8n-seg                         | [1, 3, 640, 640]        | INT8     | 28.9                  | 33.8      | 63.2           | 76.8      | 66.0 | 18.6       | 27.6       | 32.9       |
|                  | yolov8s-seg                         | [1, 3, 640, 640]        | INT8     | 12.0                  | 14.1   | 29.4           | 31.2      | 27.1 | 6.6        | 9.8        | 14.6       |
|                  | yolov8m-seg                         | [1, 3, 640, 640]        | INT8     | 5.2                   | 6.3       | 12.6           | 12.6      | 11.2 | 3.1        | 4.6        | 6.9        |
| ppseg            | ppseg_lite_1024x512                 | [1, 3, 512, 512]        | INT8     | 6.6                | 14.1   | 42.7   | 38.4 | 13.0 | 18.4       | 27.1       | 20.9       |
| mobilesam        | mobilesam_encoder_tiny              | [1, 3, 448, 448]        | FP16     | 0.8                   | 6.9       | 10.3           | 10.6      | 9.5 | /          | /          | /          |
|                  | mobilesam_decoder                   | [1, 1, 112, 112]        | FP16     | 24.5                  | 70.6      | 119.4          | 106.6     | 84.5 | /          | /          | /          |
| RetinaFace       | RetinaFace_mobile320                | [1, 3, 320, 320]        | INT8     | 148.9            | 352.9 | 229.4      | 501.8 | 451.6 | 144.8      | 212.5      | 198.5      |
|                  | RetinaFace_resnet50_320             | [1, 3, 320, 320]        | INT8     | 18.4                  | 27.1      | 50.0           | 54.6      | 46.5 | 14.6       | 20.8       | 24.6       |
| LPRNet           | lprnet                              | [1, 3, 24, 94]          | FP16     | 153.6             | 455.0 | 605.3      | 690.5 | 568.4 | 30.6(INT8) | 47.6(INT8) | 30.1(INT8) |
| PPOCR-Det        | ppocrv4_det                         | [1, 3, 480, 480]        | INT8     | 23.2                  | 28.5      | 51.6     | 68.4      | 52.4 | 11.0       | 16.1       | 14.2       |
| PPOCR-Rec        | ppocrv4_rec                         | [1, 3, 48, 320]         | FP16     | 19.1                  | 54.6      | 78.5           | 105.5      | 79.7 | 1.0        | 1.6        | 6.7        |
| lite_transformer | lite-transformer-encoder-16         | embedding-256, token-16 | FP16     | 385.9             | 858.2 | 986.9      | 980.5 | 853.3 | 22.7       | 35.4       | 98.3       |
|                  | lite-transformer-decoder-16         | embedding-256, token-16 | FP16     | 148.0             | 306.9   | 372.2 | 301.2 | 256.8 | 48.0       | 65.8       | 109.9      |
| clip             | clip_images                         | [1, 3, 224, 224]        | FP16     | 2.3                   | 3.4       | 6.9           | 6.3       | 5.9 | /          | /          | /          |
|                  | clip_text                           | [1, 20]                 | FP16     | 30.8                  | 71.4   | 96.1       | 64.3      | 61.0 | /          | /          | /          |
| wav2vec2         | wav2vec2_base_960h_20s              | 20s audio               | FP16     | RTF <br>0.839 | RTF <br>0.129 | RTF <br>0.095 | RTF <br>0.076  | RTF<br/>0.090 | /        | /        | /         |
| whisper          | whisper_base_20s                    | 20s audio               | FP16     | RTF <br>1.207   | RTF <br>0.360   | RTF <br>0.196  | RTF <br>0.224  | RTF<br/>0.305 | /        | /        | /         |
| zipformer        | zipformer-bilingual-zh-en-t         | streaming audio         | FP16     | RTF <br>0.183   | RTF <br>0.094 | RTF <br>0.059  | RTF <br>0.072  | RTF<br/>0.106 | /        | /        | /         |
| yamnet           | yamnet_3s                           | 3s audio                | FP16     | RTF <br>0.011  | RTF <br>0.006  | RTF <br>0.003 | RTF <br>0.004 | RTF<br/>0.005 | /        | /        | /         |
| mms_tts          | mms_tts_eng_200                     | token-200               | FP16     | RTF <br>0.311 | RTF <br>0.138 | RTF <br>0.108 | RTF <br>0.069 | RTF<br/>0.095 | /        | /        | /         |





- 该性能数据基于各平台的最大NPU频率进行测试
- 该性能数据指模型推理的耗时, 不包含前后处理的耗时
- `/`表示当前版本暂不支持



## Demo编译说明

对于 Linux 系统的开发板:

```sh
./build-linux.sh -t <target> -a <arch> -d <build_demo_name> [-b <build_type>] [-m]
    -t : target (rk356x/rk3576/rk3588/rv1106/rv1126b/rv1126/rk1808)
    -a : arch (aarch64/armhf)
    -d : demo name
    -b : build_type(Debug/Release)
    -m : enable address sanitizer, build_type need set to Debug
Note: 'rk356x' represents rk3562/rk3566/rk3568, 'rv1106' represents rv1103/rv1106, 'rv1126' represents rv1109/rv1126，'rv1126b' is different from 'rv1126'.

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
| 2.5.0 | 更新版本号                                                   |
| 2.4.2 | 新增yolo26示例和bug修复                                      |
| 2.3.2 | 新增 `RV1126B` 平台支持                                      |
| 2.3.0 | 新增 yolo11、zipformer、mms_tts 等示例                       |
| 2.2.0 | 添加新例程 wav2vec, mobilesam. 更新部分模型的导出说明        |
| 2.1.0 | 新例程添加, 包含 yolov8_pose, yolov8_obb, yolov10, yolo_world, clip, whisper, yamnet <br>部分模型暂不支持 `RK1808`, `RV1109`, `RV1126` 平台, 将在下个版本添加支持 |
| 2.0.0 | 新增 `RK3576` 平台支持 <br />新增 `RK1808`,  `RV1109`, `RV1126` 平台支持 |
| 1.6.0 | 提供目标检测、图像分割、OCR、车牌识别等多个例程<br />支持`RK3562`, `RK3566`, `RK3568`, `RK3588`平台<br />部分支持`RV1103`, `RV1106`平台 |
| 1.5.0 | 提供Yolo检测模型的demo                                       |



## 环境依赖

RKNN Model Zoo 的例程基于当前最新的 RKNPU SDK 进行验证。若使用低版本的 RKNPU SDK 进行验证, 推理性能、推理结果可能会有差异。

| 版本  | RKNPU2 SDK | RKNPU1 SDK |
| ----- | ---------- | ---------- |
| 2.5.0 | >=2.5.0    | -          |
| 2.4.2 | >=2.4.2    | >=1.7.5    |
| 2.3.2 | >=2.3.2    | >=1.7.5    |
| 2.3.0 | >=2.3.0    | >=1.7.5    |
| 2.2.0 | >=2.2.0    | >=1.7.5    |
| 2.1.0 | >=2.1.0    | >=1.7.5    |
| 2.0.0 | >=2.0.0    | >=1.7.5    |
| 1.6.0 | >=1.6.0    | -          |
| 1.5.0 | >=1.5.0    | >=1.7.3    |



## RKNPU相关资料

- RKNPU2 SDK: https://github.com/airockchip/rknn-toolkit2
- RKNPU1 SDK: https://github.com/airockchip/rknn-toolkit



## 许可证

[Apache License 2.0](./LICENSE)

