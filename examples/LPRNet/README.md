# LPRNet

## Table of contents

- [Table of contents](#table-of-contents)
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
    - [7.1 Compile and Build](#71-compile-and-build)
    - [7.2 Push demo files to device](#72-push-demo-files-to-device)
    - [7.3 Run demo](#73-run-demo)
- [8. Expected Results](#8-expected-results)



## 1. Description

The model used in this example comes from the following open source projects:  

https://github.com/sirius-ai/LPRNet_Pytorch/



## 2. Current Support Platform

RK3566, RK3588, RK3568, RK3562, RK1808, RV1109, RV1126



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
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: python convert.py ../model/lprnet.onnx rk3588
# output model will be saved as ../model/lprnet.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<dtype>(optional)`: Specify as `i8`, `u8` or `fp`, `i8`/`u8` means to do quantization, `fp` means no to do quantization, default is `i8`/`u8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `lprnet.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with RKNN model
python lprnet.py --model_path <rknn_model> --target <TARGET_PLATFORM>
```
*Description:*
- <TARGET_PLATFORM>: Specified as the NPU platform name. Such as 'rk3588'.
- <rknn_model>: Specified as the model path.

*The expected results are as follows:*
```
车牌识别结果: 湘F6CL03
```

## 6. Android Demo
**Note: RK1808, RV1109, RV1126 does not support Android.**

#### 6.1 Compile and Build

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#android-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.

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

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#linux-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.

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
