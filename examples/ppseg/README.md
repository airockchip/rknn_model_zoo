# ppseg

## Table of contents

- [1. Description](#1-description)
- [2. Current Support Platform](#2-current-support-platform)
- [3. Pretrained Model](#3-pretrained-model)
- [4. Convert to RKNN](#4-convert-to-rknn)
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

https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B, RV1109, RV1126, RK1808, RK3399PRO


## 3. Pretrained Model

Download link: 

[pp_liteseg_cityscapes.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppseg/pp_liteseg_cityscapes.onnx)

Download with shell command:

```
cd model
./download_model.sh
```

**(Optional) Paddle to ONNX**: Refer [Paddle_2_ONNX.md](./Paddle_2_ONNX.md)



## 4. Convert to RKNN

*Usage:*

```shell
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python convert.py ../model/pp_liteseg_cityscapes.onnx rk3588
# output model will be saved as ../model/pp_liteseg_cityscapes.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `pp_liteseg_cityscapes.rknn`



## 5. Android Demo

#### 5.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d ppseg

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d ppseg
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_ppseg_demo/ /data/
```

#### 5.3 Run demo

```sh
adb shell
cd /data/rknn_ppseg_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppseg_demo model/pp_liteseg.rknn model/test.png
```

- After running, the result was saved as `result.png`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_ppseg_demo/result.png
  ```



## 6. Linux Demo

#### 6.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d ppseg

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d ppseg
# such as 
./build-linux.sh -t rv1126 -a armhf -d ppseg
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
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_ppseg_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_ppseg_demo/` to `userdata`.

#### 6.3 Run demo

```sh
adb shell
cd /userdata/rknn_ppseg_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppseg_demo model/pp_liteseg.rknn model/test.png
```

- After running, the result was saved as `result.png`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /userdata/rknn_ppseg_demo/result.png
  ```



## 7. Expected Results

<img src="./result.png">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.