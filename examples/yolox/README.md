# yolox

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
  - [7.1 Compile and Build](#71-compile-and-build)
  - [7.2 Push demo files to device](#72-push-demo-files-to-device)
  - [7.3 Run demo](#73-run-demo)
- [8. Expected Results](#8-expected-results)



## 1. Description

The model used in this example comes from the following open source projects:  

https://github.com/airockchip/YOLOX



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3562



## 3. Pretrained Model

Download link: 

[./yolox_s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/yolox/yolox_s.onnx)<br />[./yolox_m.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/yolox/yolox_m.onnx)

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
python convert.py ../model/yolox_s.onnx rk3588
# output model will be saved as ../model/yolox.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `yolox.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with PyTorch model or ONNX model
python yolox.py --model_path <pt_model/onnx_model> --img_show

# Inference with RKNN model
python yolox.py --model_path <rknn_model> --target <TARGET_PLATFORM> --img_show
```

*Description:*

- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).

- `<pt_model / onnx_model / rknn_model>`: Specify the model path.



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d yolox

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d yolox
```

*Description:*

- `<android_ndk_path>`: Specify Android NDK path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_yolox_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_yolox_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolox_demo model/yolox.rknn model/bus.jpg
```

- After running, the result was saved as `result.png`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_yolox_demo/result.png
  ```

- Output result refer [Expected Results](#8-expected-results).



## 7. Linux Demo

#### 7.1 Compile and Build

*Usage:*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d yolox

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d yolox
```

*Description:*

- `<GCC_COMPILER_PATH>`: Specified as GCC_COMPILER path.
- `<TARGET_PLATFORM>` : Specify NPU platform name. Support Platform refer [here](#2 Current Support Platform).
- `<ARCH>`: Specify device system architecture. To query device architecture, refer to the following command: 
  
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```

#### 7.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolox_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolox_demo/` to `userdata`.

#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_yolox_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolox_demo model/yolox.rknn model/bus.jpg
```

- After running, the result was saved as `result.png`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /userdata/rknn_yolox_demo/result.png
  ```

- Output result refer [Expected Results](#8-expected-results).



## 8. Expected Results

This example will print the labels and corresponding scores of the test image detection results, as follows:

```
bus @ (91 138 549 427) 0.933
person @ (105 234 220 538) 0.901
person @ (210 241 284 506) 0.878
person @ (475 236 560 520) 0.824
person @ (79 327 118 518) 0.508
```

<img src="result.png">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.