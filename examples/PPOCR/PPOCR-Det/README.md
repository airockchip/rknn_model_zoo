# PPOCR-Det

## Current Support Platform

RK3566, RK3568, RK3588, RK3562, RK1808, RV1109, RV1126


## Download ONNX model

Download link: 

[ppocrv4_det.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/PPOCR/ppocrv4_det.onnx)

Download with shell command:

```
cd model
./download_model.sh
```



## (Optional)PADDLE to ONNX

Please refer [Paddle_2_ONNX.md](Paddle_2_ONNX.md) 



## Model Convert

*Usage:*

```
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>
# such as: python convert.py ../model/ppocrv4_det.onnx rk3566

# output model will be saved as ../model/ppocrv4_det.rknn
```

*Description:*

- <onnx_model> should be the ONNX model path.
- <TARGET_PLATFORM>  could be specified as RK3562, RK3566, RK3568, RK3588, RK1808, RV1109, RV1126 according to board SOC version.
- <dtype\> is *optional*, could be specified as `i8`, `u8` or `fp`, `i8`/`u8` means to do quantization, `fp` means no to do quantization, default is `i8`/`u8`.
- <output_rknn_path> is **optional**, used to specify the saving path of the RKNN model, default save path is `../model/ppocrv4_det.rknn`

*Attention:*

- mmse quantized_algorithm can improve precision of ppocrv4_det rknn model, while it will increase the convert time.

- Our experiment show mmse can bring +0.7%@precision / +1.7%recall / +1.2%@hmean improvement compared to normal quantized_algorithm.


## Python Demo

*Usage:*

```shell
cd python

# Inference with ONNX model
python ppocr_det.py --model_path <onnx_model>
# such as: python ppocr_det.py --model_path ../model/ppocrv4_det.onnx 

# Inference with RKNN model
python ppocr_det.py --model_path <rknn_model> --target <TARGET_PLATFORM>
# such as: python ppocr_det.py --model_path ../model/ppocrv4_det.rknn --target rk3588
```
*Description:*
- <TARGET_PLATFORM>: Specify NPU platform name. Such as 'rk3588'.

- <onnx_model / rknn_model>: specified as the model path.


## Android Demo
**Note: RK1808, RV1109, RV1126 does not support Android.**

### Compiling && Building

Modify the path of Android NDK in `build-android.sh`.

For example,

```sh
export ANDROID_NDK_PATH=~/opt/toolchain/android-ndk-r18b
```

Then, run this script:

```sh
./build-android.sh -t <TARGET_PLATFORM> -a arm64-v8a -d PPOCR-Det

# such as
./build-android.sh -t rk3588 -a arm64-v8a -d PPOCR-Det 
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board

Connect the USB port to PC, then push all demo files to the board.

```sh
adb root
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_PPOCR-Det_demo/ /data/
```

### Running

```sh
adb shell
cd /data/rknn_PPOCR-Det_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_det_demo model/ppocrv4_det.rknn model/test.jpg
```

## Aarch64 Linux Demo

### Compiling && Building

According to the target platform, modify the path of 'GCC_COMPILER' in 'build-linux.sh'.

```sh
export GCC_COMPILER=/opt/tools/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
```

Then, run the script:

```sh
./build-linux.sh  -t <TARGET_PLATFORM> -a aarch64 -d PPOCR-Det
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board


Push `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Det_demo` to the board,

- If use adb via the EVB board:

```
adb push install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Det_demo /userdata/
```

- For other boards, use the scp or other different approaches to push all files under `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Det_demo` to `/userdata`.

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Running

```sh
adb shell
cd /data/rknn_PPOCR-Det_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_det_demo model/ppocrv4_det.rknn model/test.jpg
```

Note: Try searching the location of librga.so and add it to LD_LIBRARY_PATH if the librga.so is not found in the lib folder.
Use the following command to add it to LD_LIBRARY_PATH.

```sh
export LD_LIBRARY_PATH=./lib:<LOCATION_LIBRGA>
```

## Expected Results
This example will print the box and corresponding scores of the test image detect results, as follows:
```
[0]: [(27, 459), (136, 459), (136, 478), (27, 478)] 0.990687
[1]: [(27, 428), (371, 428), (371, 446), (27, 446)] 0.865046
[2]: [(25, 397), (361, 395), (362, 413), (26, 415)] 0.947012
[3]: [(367, 368), (474, 366), (476, 386), (368, 388)] 0.988151
[4]: [(27, 363), (282, 365), (281, 384), (26, 382)] 0.952980
[5]: [(25, 334), (342, 334), (342, 352), (25, 352)] 0.955272
...
```

<img src="result.jpg">

<br>

- Note: Different platforms, different versions of tools and drivers may have slightly different results.