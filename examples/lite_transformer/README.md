# lite_transformer

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
  - [6.1 Compile and Build](#61-compile-and-build)
  - [6.2 Push demo files to device](#62-push-demo-files-to-device)
  - [6.3 Run demo](#63-run-demo)
- [7. Expected Results](#7-expected-results)



## 1. Description

The model used in this example comes from the following open source projects:  

https://github.com/airockchip/lite-transformer



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3562



## 3. Pretrained Model

Download link: 

[lite-transformer-encoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/lite_transformer/lite-transformer-encoder-16.onnx)<br />[lite-transformer-decoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/lite_transformer/lite-transformer-decoder-16.onnx)

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
python convert.py ../model/lite-transformer-encoder-16.onnx rk3588
# output model will be saved as ../model/lite-transformer-encoder-16.rknn

python convert.py ../model/lite-transformer-decoder-16.onnx rk3588
# output model will be saved as ../model/lite-transformer-decoder-16.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `fp`. Currently not support `i8` lite transformer model in this version.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model name with `rknn` suffix.



## 5. Android Demo

#### 5.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d lite_transformer

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d lite_transformer
```

*Description:*
- `<android_ndk_path>`: Specify Android NDK path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_lite_transformer_demo/ /data/
```

#### 5.3 Run demo

```sh
adb shell
cd /data/rknn_lite_transformer_demo

export LD_LIBRARY_PATH=./lib
./rknn_lite_transformer_demo model/lite-transformer-encoder-16.rknn model/lite-transformer-decoder-16.rknn thank you
```

- The output result refer [Expected Results](#7-expected-results).




## 6. Linux Demo

#### 6.1 Compile and Build

*Usage:*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d lite_transformer

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d lite_transformer
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 6.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_lite_transformer_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_lite_transformer_demo/` to `userdata`.

#### 6.3 Run demo

```sh
adb shell
cd /userdata/rknn_lite_transformer_demo

export LD_LIBRARY_PATH=./lib
./rknn_lite_transformer_demo model/lite-transformer-encoder-16.rknn model/lite-transformer-decoder-16.rknn thank you
```

- The output result refer [Expected Results](#7-expected-results).



## 7. Expected Results

```
#./rknn_lite_transformer_demo model/lite-transformer-encoder-16.rknn model/lite-transformer-decoder-16.rknn thank you

bpe preprocess use: 0.063000 ms
rknn encoder run use: 2.037000 ms
rknn decoder once run use: 6.686000 ms
decoder run 4 times. cost use: 30.348000 ms
inference time use: 33.730999 ms
output_strings: 感谢你
```

