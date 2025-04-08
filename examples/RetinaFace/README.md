# RetinaFace

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

The model used in this example comes from the following open source projects:  

https://github.com/biubug6/Pytorch_Retinaface



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B, RV1109, RV1126, RK1808, RK3399PRO


## 3. Pretrained Model

Download link: 

[RetinaFace_mobile320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_mobile320.onnx)<br />[RetinaFace_resnet50_320.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/RetinaFace/RetinaFace_resnet50_320.onnx)

Download with shell command:

```
cd model
./download_model.sh
```



## 4. Convert to RKNN

*Usage:*

```shell
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python convert.py ../model/RetinaFace_mobile320.onnx rk3588
# output model will be saved as ../model/RetinaFace_mobile320.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `RetinaFace_mobile320.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with RKNN model
python RetinaFace.py --model_path <rknn_model> --target <TARGET_PLATFORM>
# The inference result will be saved as the image result.jpg.
```
*Description:*
- <TARGET_PLATFORM>: Specified as the NPU platform name. Such as 'rk3588'.
- <rknn_model>: Specified as the model path.



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d RetinaFace

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d RetinaFace
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_RetinaFace_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_RetinaFace_demo

export LD_LIBRARY_PATH=./lib
./rknn_retinaface_demo model/RetinaFace_mobile320.rknn model/test.jpg
```

- After running, the result was saved as `result.jpg`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_RetinaFace_demo/result.jpg
  ```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d RetinaFace

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d RetinaFace
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
    ```sh
    export GCC_COMPILER=~/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf
    ```
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 7.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_RetinaFace_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_RetinaFace_demo/` to `userdata`.

#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_RetinaFace_demo

export LD_LIBRARY_PATH=./lib
./rknn_retinaface_demo model/RetinaFace_mobile320.rknn model/test.jpg
```

- After running, the result was saved as `result.jpg`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /userdata/rknn_RetinaFace_demo/result.jpg
  ```

- Note: 
  1. For the generation of BOX_PRIORS_320 and BOX_PRIORS_640 in C demo post-processing, please refer to the PriorBox function in python/RetinaFace.py. This function aims to pre-generate the anchors box parameters. In order to speed up the demo post-processing, the C code directly generates the array.
  2. In C demo post-processing, num_priors is the number of anchor boxes. When the model shape is 320x320, num_priors is 4200. When the model shape is 640x640, num_priors is 16800.



## 8. Expected Results

This example will print the labels and corresponding scores of the test image detect results, as follows:
```
face @(302 76 476 300) score=0.999512
```
<img src="python/result.jpg">


- Note: Different platforms, different versions of tools and drivers may have slightly different results.