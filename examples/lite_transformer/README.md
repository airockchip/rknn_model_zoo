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

RK3566, RK3568, RK3588, RK3562, RK1808, RV1109, RV1126



## 3. Pretrained Model

Download link: 

[lite-transformer-encoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-encoder-16.onnx)<br />[lite-transformer-decoder-16.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/lite_transformer/lite-transformer-decoder-16.onnx)

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
- `<dtype>(optional)`: Specify as `i8`/`u8`, `fp`. `i8`/`u8` for doing quantization, `fp` for no quantization. Default is `fp`. Currently not support `i8`/`u8` lite transformer model in this version.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model name with `rknn` suffix.



## 5. Android Demo

**Note: RK1808, RV1109, RV1126 does not support Android.**

#### 5.1 Compile and Build

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#android-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.  
**Note: Please replace the model name with `lite_transformer`.**

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

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#linux-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.  
**Note: Please replace the model name with `lite_transformer`.**

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

