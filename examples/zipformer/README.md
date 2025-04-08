# zipformer

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

Chinese-English ASR model using k2-zipformer-streaming.

The model used in this example comes from the following open source projects:  

https://huggingface.co/csukuangfj/k2fsa-zipformer-bilingual-zh-en-t



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B


## 3. Pretrained Model

Download link: 

[encoder-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/encoder-epoch-99-avg-1.onnx)<br />[decoder-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/decoder-epoch-99-avg-1.onnx)<br />[joiner-epoch-99-avg-1.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/zipformer/joiner-epoch-99-avg-1.onnx)

Download with shell command:

```sh
cd model
./download_model.sh
```

**Note: For exporting zipformer onnx models, please refer to [export-for-onnx.sh](https://huggingface.co/csukuangfj/k2fsa-zipformer-bilingual-zh-en-t/blob/main/exp/96/export-for-onnx.sh)**


## 4. Convert to RKNN

*Usage:*

```shell
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python convert.py ../model/encoder-epoch-99-avg-1.onnx rk3588
# output model will be saved as ../model/encoder-epoch-99-avg-1.rknn

python convert.py ../model/decoder-epoch-99-avg-1.onnx rk3588
# output model will be saved as ../model/decoder-epoch-99-avg-1.rknn

python convert.py ../model/joiner-epoch-99-avg-1.onnx rk3588
# output model will be saved as ../model/joiner-epoch-99-avg-1.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `fp`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model.



## 5. Python Demo

*Usage:*

```shell
# Install kaldifeat
# Refer to https://csukuangfj.github.io/kaldifeat/installation/from_wheels.html for installation.
# This python demo is tested under version: kaldifeat-1.25.4.dev20240223

cd python
# Inference with ONNX model
python zipformer.py --encoder_model_path <onnx_model> --decoder_model_path <onnx_model> ---joiner_model_path <onnx_model>

# Inference with RKNN model
python zipformer.py --encoder_model_path <rknn_model> --decoder_model_path <rknn_model> --joiner_model_path <rknn_model> --target <TARGET_PLATFORM>
```
*Description:*
- <TARGET_PLATFORM>: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- <onnx_model / rknn_model>: Specify model path.



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d zipformer

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d zipformer
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_zipformer_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_zipformer_demo

export LD_LIBRARY_PATH=./lib
./rknn_zipformer_demo model/encoder-epoch-99-avg-1.rknn model/decoder-epoch-99-avg-1.rknn model/joiner-epoch-99-avg-1.rknn model/test.wav
```



## 7. Linux Demo

Please note that the Linux compilation tool chain recommends using `gcc-linaro-6.3.1(aarch64)/gcc-arm-8.3(armhf)/armhf-uclibcgnueabihf(armhf for RV1106/RV1103 series)`. Using other versions may encounter the problem of Cdemo compilation failure. For detailed compilation guide, please refer to [Compilation_Environment_Setup_Guide.md](../../docs/Compilation_Environment_Setup_Guide.md)

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d zipformer

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d zipformer
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
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_zipformer_demo/ /data/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_zipformer_demo/` to `data`.

#### 7.3 Run demo

```sh
adb shell
cd /data/rknn_zipformer_demo

export LD_LIBRARY_PATH=./lib
./rknn_zipformer_demo model/encoder-epoch-99-avg-1.rknn model/decoder-epoch-99-avg-1.rknn model/joiner-epoch-99-avg-1.rknn model/test.wav
```


## 8. Expected Results

This example will print the recognized text, as follows:
```
Zipformer output: 对我做了介绍那么我想说的是大家如果对我的研究感兴趣呢
```

- Note: Different platforms, different versions of tools and drivers may have slightly different results.