# PPOCR-System

## Table of contents

- [1. Description](#1-description)
- [2. Current Support Platform](#2-current-support-platform)
- [3. Prepare Model](#3-prepare-model)
- [4. Python Demo](#4-python-demo)
- [5. Android Demo](#5-android-demo)
  - [5.1 Compile and Build](#51-compile-and-build)
  - [5.2 Push demo files to device](#52-push-demo-files-to-device)
  - [5.3 Run demo](#53-run-demo)
- [6. Linux Demo](#6-linux-demo)
  - [6.1 Compile \&\& Build](#61-compile-and-build)
  - [6.2 Push demo files to device](#62-push-demo-files-to-device)
  - [6.3 Run demo](#63-run-demo)
- [7. Expected Results](#7-expected-results)



## 1. Description

The model used in this example comes from the following open source projects:  

https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.7



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3576, RK3562, RV1109, RV1126, RK1808, RK3399PRO



## 3. Prepare model

Refer [PPOCR-Det](../PPOCR-Det) and [PPOCR-Rec](../PPOCR-Rec) to get ONNX and RKNN models.



## 4. Python Demo

*Usage:*

```shell
cd python

# Inference with ONNX model
python ppocr_system.py --det_model_path <onnx_model> --rec_model_path <onnx_model>
# such as: python ppocr_system.py --det_model_path ../../PPOCR-Det/model/ppocrv4_det.onnx --rec_model_path ../../PPOCR-Rec/model/ppocrv4_rec.onnx

# Inference with RKNN model
python ppocr_system.py --det_model_path <rknn_model> --rec_model_path <rknn_model> --target <TARGET_PLATFORM>
# such as: python ppocr_system.py --det_model_path ../../PPOCR-Det/model/ppocrv4_det.rknn --rec_model_path ../../PPOCR-Rec/model/ppocrv4_rec.rknn --target rk3588
```

*Description:*

- `<TARGET_PLATFORM>`: Specify NPU platform name. Such as 'rk3588'.

- `<onnx_model / rknn_model>`: specified as the model path.



## 5. Android Demo

#### 5.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d PPOCR-System

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d PPOCR-System
```

*Description:*
- `<android_ndk_path>`: Specify Android NDK path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command:
	```shell
	# Query architecture. For Android, ['arm64-v8a' or 'armeabi-v7a'] should shown in log.
	adb shell cat /proc/version
	```

#### 5.2 Push demo files to device

With device connected via USB port, push demo files to devices:

```shell
adb root
adb remount
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_PPOCR-System_demo/ /data/
```

#### 5.3 Run demo

```sh
adb shell
cd /data/rknn_PPOCR-System_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_system_demo ppocrv4_det_i8.rknn ppocrv4_rec_fp16.rknn model/test.jpg
```



## 6. Linux Demo

#### 6.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d PPOCR-System

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d PPOCR-System
# such as 
./build-linux.sh -t rv1106 -a armhf -d PPOCR-System
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.

- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).

- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 6.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_PPOCR-System_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_PPOCR-System_demo/` to `userdata`.

#### 6.3 Run demo

```sh
adb shell
cd /userdata/rknn_PPOCR-System_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_system_demo ppocrv4_det_i8.rknn ppocrv4_rec_fp16.rknn model/test.jpg
```

- Note: Try searching the location of librga.so and add it to LD_LIBRARY_PATH if the librga.so is not found in the lib folder.
  Use the following command to add it to LD_LIBRARY_PATH.
  
- ```
  export LD_LIBRARY_PATH=./lib:<LOCATION_LIBRGA>
  ```



## 7. Expected Results

This example will print the box and text recognition results of the test image, as follows:

```
[0] @ [(28, 37), (302, 39), (301, 70), (27, 69)]
regconize result: 纯臻营养护发素, score=0.711147
[1] @ [(26, 82), (171, 82), (171, 104), (26, 104)]
regconize result: 产品信息/参数, score=0.709542
[2] @ [(27, 111), (332, 113), (331, 134), (26, 133)]
regconize result: （45元/每公斤，100公斤起订）, score=0.697409
[3] @ [(28, 142), (281, 144), (280, 163), (27, 161)]
regconize result: 每瓶22元，1000瓶起订）, score=0.702602
[4] @ [(25, 179), (298, 177), (300, 194), (26, 195)]
regconize result: 【品牌】：代加工方式/OEMODM, score=0.705021
[5] @ [(26, 209), (234, 209), (234, 227), (26, 227)]
regconize result: 【品名】：纯臻营养护发素, score=0.710205
...
```

<img src="./result.jpg">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.