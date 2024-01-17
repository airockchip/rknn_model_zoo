# LPRNet

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

https://github.com/sirius-ai/LPRNet_Pytorch/



## 2. Current Support Platform

RK3566, RK3588, RK3568, RK3562



## 3. Pretrained Model

Download link: 

[./lprnet.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/LPRNet/lprnet.onnx)

Download with shell command:

```
cd model
./download_model.sh
```



## 4. Convert to RKNN

*Usage:*

```shell
cd python
python lprnet.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python lprnet.py ../model/lprnet.onnx rk3588
# output model will be saved as ../model/lprnet.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `lprnet.rknn`



## 5. Python Demo



Please refer [Convert to RKNN](#4. Convert to RKNN). Executing the `lprnet.py`  will identify the model/test.jpg license plate. The expected results are as follows:

```
车牌识别结果: 湘F6CL03
```



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d LPRNet

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d LPRNet
```

*Description:*
- `<android_ndk_path>`: Specify Android NDK path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command:
	```shell
	# Query architecture. For Android, ['arm64-v8a' or 'armeabi-v7a'] should shown in log.
	adb shell cat /proc/version
	```

#### 6.2 Push demo files to device

With device connected via USB port, push demo files to devices:

```shell
adb root
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_LPRNet_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_LPRNet_demo

export LD_LIBRARY_PATH=./lib
./rknn_lprnet_demo model/lprnet.rknn model/test.jpg
```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d LPRNet

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d LPRNet
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 7.2 Push demo files to device

Push `install/<TARGET_PLATFORM>_linux_<ARCH>` to the board:

- If use adb via the EVB board:

    With device connected via USB port, push demo files to devices:

    ```shell
    adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_LPRNet_demo/ /userdata/
    ```

- For other boards, use the scp or other different approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>` to /userdata

Please use the specific platform instead of <TARGET_PLATFORM> above.



#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_LPRNet_demo

export LD_LIBRARY_PATH=./lib
./rknn_lprnet_demo model/lprnet.rknn model/test.jpg
```

## 8. Expected Results


This example will print the recognition result of license plate, as follows:
```
车牌识别结果: 湘F6CL03
```
- Note: Different platforms, different versions of tools and drivers may have slightly different results.
