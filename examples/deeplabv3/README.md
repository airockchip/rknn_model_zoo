# deeplabv3

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

This is the deploy example of [Deeplabv3](https://arxiv.org/abs/1706.05587) model.



## 2. Current Support Platform

RK3566, RK3568, RK3588, RK3562, RK3576, RV1109, RV1126, RK1808, RK3399PRO



## 3. Pretrained Model

Download link: 

[./deeplab-v3-plus-mobilenet-v2.pb](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/deeplabv3/deeplab-v3-plus-mobilenet-v2.pb)

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
python convert.py ../model/deeplab-v3-plus-mobilenet-v2.pb rk3588
# output model will be saved as ../model/deeplab-v3-plus-mobilenet-v2.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Support Platform refer [here](#2-current-support-platform).
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `deeplab-v3-plus-mobilenet-v2.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
python deeplabv3.py --model_path <rknn_model> --target <TARGET_PLATFORM>
# such as: python deeplabv3.py ../model/deeplab-v3-plus-mobilenet-v2.pb rk3566
# output model will be saved as ../model/deeplab-v3-plus-mobilenet-v2.rknn
```

*Description:*

- <TARGET_PLATFORM>: Specified as the NPU platform name. Such as 'rk3588'.
- <rknn_model>: Specified as the model path.


**Note**: Due to post-processing for reszie and argmax, the model needs to be cropped to run on the C demo. This is shown below by python scripts.

```py
rknn.load_tensorflow(ori_model, 
                        inputs=['sub_7'],
                        outputs=['logits/semantic/BiasAdd'],
                        input_size_list=[[1,513,513,3]])
```

Where `logits/semantic/BiasAdd` are selected as output node for deeplabv3 model rather than the original model output node.

*Expected Results*

This example will print the segmentation result on the testing image, as follows:

![result](./reference_results/python_demo_result.png)



## 6. Android Demo

#### 6.1 Compile and Build

*Usage:*

```sh
# go back to the rknn_model_zoo root directory
cd ../../
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d deeplabv3

# such as 
./build-android.sh -t rk3588 -a arm64-v8a -d deeplabv3
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
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_deeplabv3_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_deeplabv3_demo
cp /vendor/lib64/libOpenCL.so ./lib/

export LD_LIBRARY_PATH=./lib
./rknn_deeplabv3_demo model/deeplab-v3-plus-mobilenet-v2.rknn model/test_image.jpg
```

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_deeplabv3_demo/out.png
  ```



## 7. Linux Demo

#### 7.1 Compile and Build

*usage*

```shell
# go back to the rknn_model_zoo root directory
cd ../../

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
(optional)export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d deeplabv3

# such as 
./build-linux.sh -t rk3588 -a aarch64 -d deeplabv3
# such as 
./build-linux.sh -t rv1126 -a armhf -d deeplabv3
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
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_deeplabv3_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_deeplabv3_demo/` to `userdata`.

#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_deeplabv3_demo

export LD_LIBRARY_PATH=./lib
./rknn_deeplabv3_demo model/deeplab-v3-plus-mobilenet-v2.rknn model/test_image.jpg
```

- **For error message: can't find libOpenCL.so**

  This error can be fixed by creating a soft link to ARM mali library **/usr/lib/aarch64-linux-gnu/libmali.so.1.9.0**

  ```
  ln -s /usr/lib/aarch64-linux-gnu/libmali.so.1.9.0 libOpenCL.so
  ```

  Then copy libOpenCL.so to path of your lib in demo and run it again.   
  Note: **If the libmali.so cannot be found on the path of /usr/lib, please try to search this library in whole system or upgrade firmware to make sure there is a file named libmali.so on the board.**

  Note: **RK1808, RV1109, and RV1126 platforms do not have a GPU. The CPP demo under the `cpp/rknpu1` folder do not use GPU implementation, so there won't be an issue of not finding libOpenCL.so.**

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /userdata/rknn_deeplabv3_demo/out.png
  ```

- **if user wants to use other deeplabv3 model with differrnt task, please modify the following variables.**

  ```C++
  // Modify it accroding to required label for your model
  const size_t NUM_LABEL = 21;  
  ```

  Also. if user need to have different color table for converting model output to RGB or other color space, the FULL_COLOR_MAP should be modified according to your color table used in the post-processing. 



## 8. Expected Results

The expected result comes from C Demo.

![result](./reference_results/c_demo_result.png)

Note: Different platforms, different versions of tools and drivers may have slightly different results.

