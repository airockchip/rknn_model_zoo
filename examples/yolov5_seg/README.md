# yolov5_seg

## Table of contents

- [1. Description](#1-description)
- [2. Current Support Platform](#2-current-support-platform)
- [3. Pretrained Model](#3-pretrained-model)
- [4. Convert to RKNN](#4-convert-to-rknn)
- [5. Python Demo](#5-python-demo)
- [6. Android Demo](#6-android-demo)
  - [6.1 Compile and Build](#61-compile-and-build)
  - [6.2 Push demo files to device](#62-push-demo-files-to-device)
  - [6.3 Run demo](#63-run-demo)
- [7. Linux Demo](#7-linux-demo)
  - [7.1 Compile \&\& Build](#71-compile-and-build)
  - [7.2 Push demo files to device](#72-push-demo-files-to-device)
  - [7.3 Run demo](#73-run-demo)
- [8. Expected Results](#8-expected-results)



## 1. Description

YOLOv5 Segmentation is a fast and accurate instance segmentation model.

The model used in this example comes from the following open source projects:  

https://github.com/airockchip/yolov5



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B, RV1109, RV1126, RK1808, RK3399PRO


## 3. Pretrained Model

Download link: 

[./yolov5n-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5n-seg.onnx)<br />[./yolov5s-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5s-seg.onnx)<br />[./yolov5m-seg.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolov5_seg/yolov5m-seg.onnx)

Download with shell command:

```
cd model
./download_model.sh
```

**Note**: The model provided here is an optimized model, which is different from the official original model. Take yolov5n-seg.onnx as an example to show the difference between them.
1. The comparison of their output information is as follows. The left is the official original model, and the right is the optimized model. The three colored boxes in the figure represent the changes in the three outputs.

<div align=center>
  <img src="./model_comparison/yolov5_seg_output_comparison.jpg" alt="Image">
</div>

2. Taking the output change [1,19200,117] -> ([1,255,80,80],[1,96,80,80]) as an example, we split the convolution of [351x64x1x1] into [255x64x1x1] and [96x64x1x1], then we remove the subsequent subgraphs from the model (the framed part in the figure) and put them into post-processing (these subgraphs are not quantification-friendly)

<div align=center>
  <img src="./model_comparison/yolov5_seg_graph_comparison.jpg" alt="Image">
</div>


## 4. Convert to RKNN

*Usage:*

```shell
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python convert.py ../model/yolov5s-seg.onnx rk3588
# output model will be saved as ../model/yolov5_seg.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8/u8` or `fp`. `i8/u8` for doing quantization, `fp` for no quantization. Default is `i8/u8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `yolov5_seg.rknn`



## 5. Python Demo

*Usage:*

```sh
cd python
# Inference with ONNX model
python yolov5_seg.py --model_path <onnx_model> --img_show

# Inference with RKNN model
python yolov5_seg.py --model_path <rknn_model> --target <TARGET_PLATFORM> --img_show

# coco mAP test
python yolov5_seg.py --model_path <rknn_model> --target <TARGET_PLATFORM> --anno_json <val_annotation> --img_folder <val_dataset> --coco_map_test
```
*Description:*
- <TARGET_PLATFORM>: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- <onnx_model / rknn_model>: Specify model path.
- <val_annotation>: Specify COCO val annotation path. 
- <val_dataset>: Specify COCO val images path.


## 6. Android Demo
**Note: RK1808, RV1109, RV1126 does not support Android.**

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d yolov5_seg

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d yolov5_seg
```

*Description:*
- `<android_ndk_path>`: Specify Android NDK path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command:
	```shell
	# Query architecture. For Android, ['arm64-v8a' or 'armeabi-v7a'] should shown in log.
	adb shell cat /proc/version
	```

#### 6.2 Push demo files to device

With device connected via USB port, push demo files to devices:

```shell
adb root
adb remount
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_yolov5_seg_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_yolov5_seg_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolov5_seg_demo model/yolov5_seg.rknn model/bus.jpg
```

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_yolov5_seg_demo/out.png
  ```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d yolov5_seg

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d yolov5_seg
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 7.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolov5_seg_demo/ /data/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolov5_seg_demo/` to `data`.

#### 7.3 Run demo

```sh
adb shell
cd /data/rknn_yolov5_seg_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolov5_seg_demo model/yolov5_seg.rknn model/bus.jpg
```

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /data/rknn_yolov5_seg_demo/out.png
  ```


## 8. Expected Results

This example will print the labels and corresponding scores of the test image detect results, as follows:
```
person @ (213 238 284 515) 0.886
person @ (109 241 224 536) 0.861
person @ (474 230 560 522) 0.842
bus @ (98 133 547 461) 0.816
person @ (79 326 124 520) 0.510
```

<img src="./reference_results/yolov5s_seg_c_demo_result.png">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.