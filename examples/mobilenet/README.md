# mobilenet

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

The model used in this example comes from the following open source model zoo:  

https://github.com/onnx/models/tree/8e893eb39b131f6d3970be6ebd525327d3df34ea/vision/classification/mobilenet/model/mobilenetv2-12.onnx



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3562, RK3576, RV1106, RV1103, RV1109, RV1126, RK1808, RK3399PRO



## 3. Pretrained Model

Download link: 

[mobilenetv2-12.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/mobilenet/mobilenetv2-12.onnx)

Download with shell command:

```
cd model
./download_model.sh
```



## 4. Convert to RKNN

*Usage:*

```shell
cd python
python mobilenet.py --model <onnx_model> --target <TARGET_PLATFORM> --output_path <output_path> --dtype <data_type>

# such as: 
python mobilenet.py --model ../model/mobilenetv2-12.onnx --target rk3588
# output model will be saved as ../model/mobilenetv2-12.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `mobilenetv2-12.rknn`



## 5. Python Demo

Same as [Step4](#4-convert-to-rknn) .



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d mobilenet

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d mobilenet
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_mobilenet_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_mobilenet_demo

export LD_LIBRARY_PATH=./lib
./rknn_mobilenet_demo model/mobilenetv2-12.rknn model/bell.jpg
```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d mobilenet

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d mobilenet
# such as 
./build-linux.sh -t rv1106 -a armhf -d mobilenet
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
  - For RV1106, RV1103, GCC_COMPILER version is `arm-rockchip830-linux-uclibcgnueabihf`
    ```sh
    export GCC_COMPILER=~/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf
    ```
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 7.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_mobilenet_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_mobilenet_demo/` to `userdata`.

#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_mobilenet_demo

export LD_LIBRARY_PATH=./lib
./rknn_mobilenet_demo model/mobilenetv2-12.rknn model/bell.jpg
```

- RV1106/1103 LD_LIBRARY_PATH must specify as the absolute path. Such as 

  ```sh
  export LD_LIBRARY_PATH=/userdata/rknn_mobilenet_demo/lib
  ```




## 8. Expected Results

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