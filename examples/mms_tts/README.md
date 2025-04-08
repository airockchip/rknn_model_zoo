# MMS-TTS

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

MMS-TTS (Massively Multilingual Speech, Text to Speech) is part of Facebook's Massively Multilingual Speech project, aiming to provide speech technology across a diverse range of languages.

The model used in this example comes from the following open source projects:  

https://huggingface.co/facebook/mms-tts-eng



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B



## 3. Pretrained Model

Download link: 

[mms_tts_eng_encoder_200.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mms_tts/mms_tts_eng_encoder_200.onnx)<br />[mms_tts_eng_decoder_200.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mms_tts/mms_tts_eng_decoder_200.onnx)

Download with shell command:

```
cd model
./download_model.sh
```

**Note: For exporting mms_tts onnx models, please refer to [export_onnx.md](./export_onnx.md)**


## 4. Convert to RKNN

*Usage:*

```shell
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>

# such as: 
python convert.py ../model/mms_tts_eng_encoder_200.onnx rk3588
# output model will be saved as ../model/mms_tts_eng_encoder_200.rknn

python convert.py ../model/mms_tts_eng_decoder_200.onnx rk3588
# output model will be saved as ../model/mms_tts_eng_decoder_200.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `fp`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model.



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with ONNX model
python mms_tts.py --encoder_model_path <onnx_model> --decoder_model_path <onnx_model>

# Inference with RKNN model
python mms_tts.py --encoder_model_path <rknn_model> --decoder_model_path <rknn_model> --target <TARGET_PLATFORM>
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

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d mms_tts

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d mms_tts
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_mms_tts_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_mms_tts_demo

export LD_LIBRARY_PATH=./lib
./rknn_mms_tts_demo model/mms_tts_eng_encoder_200.rknn model/mms_tts_eng_decoder_200.rknn '"Mister quilter is the apostle of the middle classes and we are glad to welcome his gospel."'
```

- After running, the result was saved as `output.wav`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_mms_tts_demo/output.wav
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

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d mms_tts

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d mms_tts
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
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_mms_tts_demo/ /data/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_mms_tts_demo/` to `data`.

#### 7.3 Run demo

```sh
adb shell
cd /data/rknn_mms_tts_demo

export LD_LIBRARY_PATH=./lib
./rknn_mms_tts_demo model/mms_tts_eng_encoder_200.rknn model/mms_tts_eng_decoder_200.rknn '"Mister quilter is the apostle of the middle classes and we are glad to welcome his gospel."'
```

- After running, the result was saved as `output.wav`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /data/rknn_mms_tts_demo/output.wav
  ```



## 8. Expected Results

This example will generate a wav file corresponding to the input text: "Mister quilter is the apostle of the middle classes and we are glad to welcome his gospel.", such as:

[output.wav](output.wav)


- Note: Different platforms, different versions of tools and drivers may have slightly different results.