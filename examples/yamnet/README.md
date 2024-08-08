# YAMNet

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

YAMNet is a deep net that predicts 521 audio event classes from the AudioSet-YouTube corpus it was trained on. It employs the Mobilenet_v1 depthwise-separable convolution architecture.

The model used in this example comes from the following open source projects:  

https://www.tensorflow.org/hub/tutorials/yamnet



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3562, RK3576, RV1109, RV1126, RK1808



## 3. Pretrained Model

Download link: 

[yamnet_3s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yamnet/yamnet_3s.onnx)

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
python convert.py ../model/yamnet_3s.onnx rk3588
# output model will be saved as ../model/yamnet_3s.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8/u8` or `fp`. `i8/u8` for doing quantization, `fp` for no quantization. Default is `fp`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `yamnet_3s.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with ONNX model
python yamnet.py --model_path <onnx_model> 

# Inference with RKNN model
python yamnet.py --model_path <rknn_model> --target <TARGET_PLATFORM>
```
*Description:*
- <TARGET_PLATFORM>: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).

- <onnx_model / rknn_model>: Specify model path.



## 6. Android Demo

**Note: RK1808, RV1109, RV1126 does not support Android.**


#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d yamnet

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d yamnet
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_yamnet_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_yamnet_demo

export LD_LIBRARY_PATH=./lib
./rknn_yamnet_demo model/yamnet_3s.rknn model/test.wav
```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d yamnet

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d yamnet
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
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yamnet_demo/ /data/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yamnet_demo/` to `data`.

#### 7.3 Run demo

```sh
adb shell
cd /data/rknn_yamnet_demo

export LD_LIBRARY_PATH=./lib
./rknn_yamnet_demo model/yamnet_3s.rknn model/test.wav
```


## 8. Expected Results

This example will print the predicted category of this sound, as follows:
```
The main sound is: Animal
```