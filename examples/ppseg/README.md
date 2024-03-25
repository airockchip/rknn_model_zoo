# PaddleSeg Model Demo

## Current Support Platform
RK3566, RK3568, RK3588, RK3562, RK1808, RV1109, RV1126


## Model Source

Repository: [PaddleSeg](https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8)

Download link: 

[pp_liteseg_cityscapes.onnx](https://ftzr.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ppseg/pp_liteseg_cityscapes.onnx )

Download with shell command:

```
cd model
./download_model.sh
```



## (Optional) Paddle to ONNX

Refer [Paddle_2_ONNX.md](./Paddle_2_ONNX.md)



## Model Convert

*Usage:*

```
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>
# such as: python convert.py ../model/pp_liteseg_cityscapes.onnx rk3566

# output model will be saved as ../model/pp_liteseg.rknn
```

*Description:*

- <onnx_model> should be the ONNX model path.
- <TARGET_PLATFORM>  could be specified as RK3562, RK3566, RK3568, RK3588, RK1808, RV1109, RV1126 according to board SOC version.
- <dtype\> is *optional*, could be specified as `i8`, `u8` or `fp`, `i8`/`u8` means to do quantization, `fp` means no to do quantization, default is `i8`/`u8`.
- <output_rknn_path> is *optional*, used to specify the saving path of the RKNN model, default save path is `../model/pp_liteseg.rknn`


## Android Demo
**Note: RK1808, RV1109, RV1126 does not support Android.**

### Compiling && Building

Modify the path of Android NDK in 'build-android.sh'.

For example,

```sh
ANDROID_NDK_PATH=~/opt/toolchain/android-ndk-r19c
```

Then, run this script:

```sh
./build-android.sh -t <TARGET_PLATFORM> -a arm64-v8a -d ppseg
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board

Connect the USB port to PC, then push all demo files to the board.

```sh
adb root
adb remount
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_ppseg_demo/ /data/
```

### Running

```sh
adb shell
cd /data/rknn_ppseg_demo/

export LD_LIBRARY_PATH=./lib
./rknn_ppseg_demo model/pp_liteseg.rknn model/test.png
```

### Pull result img

```
adb pull /data/rknn_ppseg_demo/result.png .
```





## Aarch64 Linux Demo

### Compiling && Building

According to the target platform, modify the path of 'GCC_COMPILER' in 'build-linux.sh'.

```sh
export GCC_COMPILER=/opt/tools/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
```

Then, run the script:

```sh
./build-linux.sh  -t <TARGET_PLATFORM> -a aarch64 -d ppseg
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board


Push install/<TARGET_PLATFORM>_linux_aarch64/rknn_ppseg_demo to the board,

- If use adb via the EVB board:

```
adb push install/<TARGET_PLATFORM>_linux_aarch64/rknn_ppseg_demo /userdata/
```

- For other boards, use the scp or other different approaches to push all files under install/install/<TARGET_PLATFORM>_linux_aarch64/rknn_ppseg_demo to '/userdata'.

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Running

```sh
adb shell
cd /userdata/rknn_ppseg_demo/

export LD_LIBRARY_PATH=./lib
./rknn_ppseg_demo model/pp_liteseg.rknn model/test.png
```

Note: Try searching the location of librga.so and add it to LD_LIBRARY_PATH if the librga.so is not found in the lib folder.
Use the following command to add it to LD_LIBRARY_PATH.

```sh
export LD_LIBRARY_PATH=./lib:<LOCATION_LIBRGA>
```

### Pull result img

```
adb pull /data/rknn_ppseg_demo/result.png ./
```

## Expected Results

<img src="./result.png">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.
