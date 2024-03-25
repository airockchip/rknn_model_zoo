# ResNet

## Model Source
The model used in this example comes from the following open source projects:  
https://github.com/onnx/models/raw/main/vision/classification/resnet/model/resnet50-v2-7.onnx

Download link: 

[resnet50-v2-7.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/ResNet/resnet50-v2-7.onnx)

Download with shell command:


```
cd model
./download_model.sh
```



## Script Usage

*Usage:*
```shell
cd python
python resnet.py <onnx_model> <TARGET_PLATFORM> [dtype(optional)] [output_rknn_path(optional)]

# such as: 
python resnet.py ../model/resnet50-v2-7.onnx rk3588
# output model will be saved as ../model/resnet50-v2-7.rknn
```
*Description:*

- <onnx_model> should be the ONNX model path.
- <TARGET_PLATFORM>  could be specified as RK3562, RK3566, RK3568, RK3588, RK1808, RV1109, RV1126 according to board SOC version.
- <dtype\> is *optional*, could be specified as `i8`, `u8` or `fp`, `i8`/`u8` means to do quantization, `fp` means no to do quantization, default is `i8`.
- <output_rknn_path> is *optional*, used to specify the saving path of the RKNN model, default save path is `../model/resnet50-v2-7.rknn`




## Android Demo

**Note: RK1808, RV1109, RV1126 does not support Android.**

### Compiling && Building

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d resnet

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d resnet
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
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_resnet_demo /data/
```

### Running

```sh
adb shell
cd /data/rknn_resnet_demo

export LD_LIBRARY_PATH=./lib
./rknn_resnet_demo model/resnet50-v2-7.rknn model/dog_224x224.jpg
```



## Linux Demo

### Compiling && Building

```sh
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d resnet

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d resnet
```
- <GCC_COMPILER_PATH>: Specified as GCC_COMPILER path.

- <TARGET_PLATFORM>: Specify NPU platform name. Such as 'rk3588'.

- <ARCH\>: Specify device system architecture.

  ```sh
  # Query architecture. For Linux, ['aarch64' or 'armhf'] shown.
  adb shell cat /proc/version
  ```

### Push demo files to device

```
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_resnet_demo/ /data/
```

### Running

```sh
adb shell
cd /data/rknn_resnet_demo

export LD_LIBRARY_PATH=./lib
./rknn_resnet_demo model/resnet50-v2-7.rknn model/dog_224x224.jpg
```

## Expected Results

This example will print the TOP5 labels and corresponding scores of the test image classification results, as follows:
```
[155] score=0.794123 class=n02086240 Shih-Tzu
[154] score=0.183054 class=n02086079 Pekinese, Pekingese, Peke
[262] score=0.011264 class=n02112706 Brabancon griffon
[152] score=0.002242 class=n02085782 Japanese spaniel
[283] score=0.001936 class=n02123394 Persian cat
```
- Note: Different platforms, different versions of tools and drivers may have slightly different results.