# MobileNet

## Model Source
The model used in this example comes from the following open source projects:  
https://github.com/onnx/models/tree/main/vision/classification/mobilenet



## Pretrained Model

Download link: 

[mobilenetv2-12.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/MobileNet/mobilenetv2-12.onnx)

Download with shell command:

```
cd model
./download_model.sh
```


## Script Usage

*Usage:*
```shell
cd python
python mobilenet.py --model <onnx_model> --target <TARGET_PLATFORM> --output_path <output_path> --dtype <data_type>
# such as: 
python mobilenet.py --model ../model/mobilenetv2-12.onnx --target rk3588
# output model will be saved as ../model/mobilenetv2-12.rknn
```
*Description:*

- <onnx_model> should be the ONNX model path.
- <TARGET_PLATFORM>  could be specified as RK3562, RK3566, RK3568, RK3588, RV1103, RV1106, RK1808, RV1109, RV1126 according to board SOC version. Case insensitive.
- <output_path> export path of RKNN model. **Optional, default is `mobilenet_v2.rknn`.**
- <data_type> quantized data type. **Optional, defaul is `i8`.** `i8`/`u8` means do quantization, `fp32` means not do quantization.


## Android Demo

**Note: RK1808, RV1109, RV1126 does not support Android.**

### Compiling && Building

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d mobilenet

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d mobilenet
```

- <android_ndk_path>: Specified as Android ndk path.

- <TARGET_PLATFORM>: Specify NPU platform name. Such as 'rk3588'.

- <ARCH\>: Specify device system architecture.

  ```sh
  # Query architecture. For Android, ['arm64-v8a' or 'armeabi-v7a'] shown.
  adb shell cat /proc/version
  ```

### Push demo files to device

With device connected via USB port, push demo files to devices:

```sh
adb root
adb remount
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_mobilenet_demo /data/
```

### Running

```sh
adb shell
cd /data/rknn_mobilenet_demo

export LD_LIBRARY_PATH=./lib
./rknn_mobilenet_demo model/mobilenet_v2.rknn model/bell.jpg
```



## Linux Demo

### Compiling && Building

```sh
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d mobilenet

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d mobilenet
# such as
./build-linux.sh -t rv1106 -a armhf -d mobilenet
# such as
./build-linux.sh -t rk1808 -a aarch64 -d mobilenet
# such as
./build-linux.sh -t rv1126 -a armhf -d mobilenet
```
- <GCC_COMPILER_PATH>: Specified as GCC_COMPILER path.

  - For RV1106, RV1103, GCC_COMPILER version is 'arm-rockchip830-linux-uclibcgnueabihf'

    ```sh
    export GCC_COMPILER=~/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf
    ```

- <TARGET_PLATFORM>: Specify NPU platform name. Such as 'rk3588'.

- <ARCH\>: Specify device system architecture.

  ```sh
  # Query architecture. For Linux, ['aarch64' or 'armhf'] shown.
  adb shell cat /proc/version
  ```

### Push demo files to device

```
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_mobilenet_demo/ /data/
```

### Running

```sh
adb shell
cd /data/rknn_mobilenet_demo

export LD_LIBRARY_PATH=./lib
./rknn_mobilenet_demo model/mobilenet_v2.rknn model/bell.jpg
```

- RV1106/1103 LD_LIBRARY_PATH must fill in an absolute path. such as

  ```sh
  export LD_LIBRARY_PATH=/data/rknn_mobilenet_demo/lib
  ```



## Expected Results

This example will print the TOP5 labels and corresponding scores of the test image classification results, as follows:
```
-----TOP 5-----
[494] score=0.990035 class=n03017168 chime, bell, gong
[469] score=0.003907 class=n02939185 caldron, cauldron
[653] score=0.001089 class=n03764736 milk can
[747] score=0.000945 class=n04023962 punching bag, punch bag, punching ball, punchball
[442] score=0.000712 class=n02825657 bell cote, bell cot
```
- Note: Different platforms, different versions of tools and drivers may have slightly different results.