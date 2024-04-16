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

| Demo<br />                                                | 算法类别   | Demo支持类型 | Pretrain model<br />                                         |
| --------------------------------------------------------- | ---------- | ------------ | ------------------------------------------------------------ |
| [mobilenet](./examples/mobilenet/README.md)               | 图像分类   | FP16/INT8    | [mobilenetv2-12.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/MobileNet/mobilenetv2-12.onnx) |
| [resnet](./examples/resnet/README.md)                     | 图像分类   | FP16/INT8    | [resnet50-v2-7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ResNet/resnet50-v2-7.onnx) |
| [yolov5](./examples/yolov5/README.md)                     | 目标检测   | FP16/INT8    | [yolov5n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5n.onnx)<br />[yolov5s_relu.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s_relu.onnx)<br />[yolov5s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5s.onnx)<br />[yolov5m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5/yolov5m.onnx) |
| [yolov6](./examples/yolov6/README.md)                     | 目标检测   | FP16/INT8    | [yolov6n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6n.onnx)<br />[yolov6s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6s.onnx)<br />[yolov6m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov6/yolov6m.onnx) |
| [yolov7](./examples/yolov7/README.md)                     | 目标检测   | FP16/INT8    | [yolov7-tiny.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7-tiny.onnx)<br />[yolov7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov7/yolov7.onnx) |
| [yolov8](./examples/yolov8/README.md)                     | 目标检测   | FP16/INT8    | [yolov8n.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8n.onnx)<br />[yolov8s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8s.onnx)<br />[yolov8m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8/yolov8m.onnx) |
| [yolox](./examples/yolox/README.md)                       | 目标检测   | FP16/INT8    | [yolox_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_s.onnx)<br />[yolox_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolox/yolox_m.onnx) |
| [ppyoloe](./examples/ppyoloe/README.md)                   | 目标检测   | FP16/INT8    | [ppyoloe_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_s.onnx)<br />[ppyoloe_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppyoloe/ppyoloe_m.onnx) |
| [deeplabv3](./examples/deeplabv3/README.md)               | 图像分割   | FP16/INT8    | [deeplab-v3-plus-mobilenet-v2.pb](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/Deeplabv3/deeplab-v3-plus-mobilenet-v2.pb) |
| [yolov5-seg](./examples/yolov5_seg/README.md)             | 图像分割   | FP16/INT8    | [yolov5n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5n-seg.onnx)<br />[yolov5s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5s-seg.onnx)<br />[yolov5m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5m-seg.onnx) |
| [yolov8-seg](./examples/yolov8_seg/README.md)             | 图像分割   | FP16/INT8    | [yolov8n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8n-seg.onnx)<br />[yolov8s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8s-seg.onnx)<br />[yolov8m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov8_seg/yolov8m-seg.onnx) |
| [ppseg](./examples/ppseg/README.md)                       | 图像分割   | FP16         | [pp_liteseg_cityscapes.onnx](https://ftzr.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppseg/pp_liteseg_cityscapes.onnx ) |
| [RetinaFace](./examples/RetinaFace/README.md)             | 人脸关键点 | INT8         | [RetinaFace_mobile320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_mobile320.onnx)<br />[RetinaFace_resnet50_320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_resnet50_320.onnx) |
| [LPRNet](./examples/LPRNet/README.md)                     | 车牌识别   | FP16/INT8    | [lprnet.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/LPRNet/lprnet.onnx) |
| [PPOCR-Det](./examples/PPOCR/PPOCR-Det/README.md)         | 文字检测   | FP16/INT8    | [ppocrv4_det.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_det.onnx) |
| [PPOCR-Rec](./examples/PPOCR/PPOCR-Rec/README.md)         | 文字识别   | FP16         | [ppocrv4_rec.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_rec.onnx) |
| [lite_transformer](./examples/lite_transformer/README.md) | 语言翻译   | FP16         | [lite-transformer-encoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-encoder-16.onnx)<br />[lite-transformer-decoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-decoder-16.onnx) |



## Model performance benchmark(FPS)

| demo             | model_name                   | inputs_shape            | dtype | RK3566<br />RK3568 | RK3562 | RK3588<br />@single_core | RK3576<br />@single_core | RK3576<br/>@single_core<br/>@sparse_weight | RV1109 | RV1126 | RK1808 |
| ---------------- | ---------------------------- | ----------------------- | ----- | ------------------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |
| mobilenet        | mobilenetv2-12               | [1, 3, 224, 224]        | INT8  | 197.4              | 266.8  | 433.0  | 452.3 | 483.9 | 213.5  | 316.5  | 168.6  |
| resnet           | resnet50-v2-7                | [1, 3, 224, 224]        | INT8  | 40.6               | 54.5   | 108.6  | 97.4 | 129.9 | 24.5   | 36.4   | 37.0   |
| yolov5           | yolov5s_relu                 | [1, 3, 640, 640]        | INT8  | 26.7               | 31.6   | 63.3   | 62.6 | 82.0 | 20.3   | 29.3   | 36.7   |
|                  | yolov5n                      | [1, 3, 640, 640]        | INT8  | 41.6               | 43.8   | 68.1   | 104.4 | 112.2 | 36.4   | 53.5   | 61.0   |
|                  | yolov5s                      | [1, 3, 640, 640]        | INT8  | 19.9               | 22.7   | 42.5   | 54.2 | 65.5 | 13.7   | 20.1   | 28.1   |
|                  | yolov5m                      | [1, 3, 640, 640]        | INT8  | 8.7                | 10.6   | 19.3   | 23.0 | 31.5 | 5.8    | 8.5    | 13.1   |
| yolov6           | yolov6n                      | [1, 3, 640, 640]        | INT8  | 50.2               | 51.5   | 93.8   | 98.6 | 136.6 | 37.7   | 56.8   | 66.4   |
|                  | yolov6s                      | [1, 3, 640, 640]        | INT8  | 15.2               | 16.8   | 34.1   | 33.1 | 55.3 | 10.9   | 16.4   | 24.0   |
|                  | yolov6m                      | [1, 3, 640, 640]        | INT8  | 7.5                | 8.0    | 17.6   | 17.0 | 27.8 | 5.7    | 8.3    | 11.4   |
| yolov7           | yolov7-tiny                  | [1, 3, 640, 640]        | INT8  | 29.9               | 34.9   | 69.7   | 70.9 | 91.8 | 15.6   | 22.5   | 37.2   |
|                  | yolov7                       | [1, 3, 640, 640]        | INT8  | 4.7                | 5.5    | 10.9   | 12.5 | 17.9 | 3.3    | 4.9    | 7.4    |
| yolov8           | yolov8n                      | [1, 3, 640, 640]        | INT8  | 35.7               | 38.5   | 59.6   | 79.5 | 95.6 | 24.1   | 36.0   | 41.9   |
|                  | yolov8s                      | [1, 3, 640, 640]        | INT8  | 15.4               | 17.1   | 32.8   | 38.7 | 52.4 | 9.0    | 13.2   | 19.1   |
|                  | yolov8m                      | [1, 3, 640, 640]        | INT8  | 6.6                | 7.5    | 14.8   | 15.9 | 23.5 | 3.9    | 5.8    | 9.1    |
| yolox            | yolox_s                      | [1, 3, 640, 640]        | INT8  | 15.5               | 17.7   | 32.9   | 36.4 | 46.7 | 10.6   | 15.7   | 22.9   |
|                  | yolox_m                      | [1, 3, 640, 640]        | INT8  | 6.7                | 8.1    | 14.8   | 16.5 | 23.2 | 4.7    | 6.8    | 10.5   |
| ppyoloe          | ppyoloe_s                    | [1, 3, 640, 640]        | INT8  | 17.5               | 19.7   | 32.9   | 30.0 | 34.4 | 11.3   | 16.4   | 21.0   |
|                  | ppyoloe_m                    | [1, 3, 640, 640]        | INT8  | 7.9                | 8.3    | 16.2   | 12.9 | 14.8 | 5.2    | 7.7    | 9.4    |
| deeplabv3        | deeplab-v3-plus-mobilenet-v2 | [1, 513, 513, 1]        | INT8  | 10.7               | 20.7   | 34.4   | 38.1 | 42.5 | 10.3   | 13.1   | 4.4    |
| yolov5_seg       | yolov5n-seg                  | [1, 3, 640, 640]        | INT8  | 33.9               | 36.3   | 58.0   | 82.4 | 92.2 | 28.7   | 41.9   | 49.6   |
|                  | yolov5s-seg                  | [1, 3, 640, 640]        | INT8  | 15.3               | 17.2   | 32.6   | 39.5 | 51.1 | 9.7    | 14.0   | 22.4   |
|                  | yolov5m-seg                  | [1, 3, 640, 640]        | INT8  | 6.8                | 8.1    | 15.2   | 17.2 | 25.1 | 4.7    | 6.9    | 10.7   |
| yolov8_seg       | yolov8n-seg                  | [1, 3, 640, 640]        | INT8  | 29.1               | 30.7   | 49.1   | 64.5 | 78.0 | 18.6   | 27.8   | 32.7   |
|                  | yolov8s-seg                  | [1, 3, 640, 640]        | INT8  | 11.8               | 11.3   | 25.4   | 29.3 | 39.7 | 6.7    | 9.8    | 14.5   |
|                  | yolov8m-seg                  | [1, 3, 640, 640]        | INT8  | 5.2                | 6.1    | 11.6   | 12.1 | 18.1 | 3.1    | 4.6    | 6.8    |
| ppseg            | ppseg_lite_1024x512          | [1, 3, 512, 512]        | INT8  | 2.6                | 4.6    | 13.0   | 8.7 | 35.5 | 18.4   | 27.2   | 14.7   |
| RetinaFace       | RetinaFace_mobile320         | [1, 3, 320, 320]        | INT8  | 142.5              | 279.5  | 234.7  | 416.0 | 396.8 | 146.3  | 210.1  | 242.2  |
|                  | RetinaFace_resnet50_320      | [1, 3, 320, 320]        | INT8  | 18.5               | 26.0   | 48.8   | 47.3 | 70.4 | 14.7   | 20.9   | 24.2   |
| LPRNet           | lprnet                       | [1, 3, 24, 94]          | INT8  | 58.2               | 119.7  | 204.4  | 130.2 | 130.6 | 30.6   | 47.8   | 30.1   |
| PPOCR-Det        | ppocrv4_det                  | [1, 3, 480, 480]        | INT8  | 24.4               | 27.5   | 43.0   | 46.1 | 47.0 | 11.1   | 16.2   | 9.1   |
| PPOCR-Rec        | ppocrv4_rec                  | [1, 3, 48, 320]         | FP16  | 20.0               | 45.1   | 35.7   | 55     | 58.9 | 1.0    | 1.6    | 6.7    |
| lite_transformer | lite-transformer-encoder-16  | embedding-256, token-16 | FP16  | 130.8              | 656.7  | 261.5  | 609.1  | 674.8 | 22.7   | 35.6   | 97.8   |
|                  | lite-transformer-decoder-16  | embedding-256, token-16 | FP16  | 114.3              | 151.3  | 164.0  | 240 | 341.8 | 49.0   | 66.3   | 114.9  |

- 该性能数据基于各平台的最大NPU频率进行测试
- 该性能数据指模型推理的耗时, 不包含前后处理的耗时
- RK3576带sparse_weight是指模型开启kernel方向稀疏化 (按照4:2) 的性能
- 注: 模型开启kernel的稀疏化 (按照4:2) 可以提升一些性能，精度可能只有些微损失（具体还需要看模型）



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
| 2.0.0 | 新增所有示例`RK3576`平台的支持 <br />支持`RK1808`,  `RK1109`, `RK1126`平台 |
| 1.6.0 | 提供目标检测、图像分割、OCR、车牌识别等多个例程<br />支持`RK3562`, `RK3566`, `RK3568`, `RK3588`平台<br />部分支持`RV1103`, `RV1106`平台 |
| 1.5.0 | 提供Yolo检测模型的demo                                       |



## 环境依赖

RKNN Model Zoo 的例程基于当前最新的 RKNPU SDK 进行验证。若使用低版本的 RKNPU SDK 进行验证, 推理性能、推理结果可能会有差异。

| 版本  | RKNPU2 SDK | RKNPU1 SDK |
| ----- | ---------- | ---------- |
| 2.0.0 | >=2.0.0    | >=1.7.5    |
| 1.6.0 | >=1.6.0    | -          |
| 1.5.0 | >=1.5.0    | >=1.7.3    |



## RKNPU相关资料

- RKNPU2 SDK: https://github.com/airockchip/rknn-toolkit2
- RKNPU1 SDK: https://github.com/airockchip/rknn-toolkit



## 许可证

[Apache License 2.0](./LICENSE)

