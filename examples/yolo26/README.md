# yolo26

## Table of contents

- [1. Description](#1-description)
- [2. Current Support Platform](#2-current-support-platform)
- [3. Pretrained Model](#3-pretrained-model)
- [4. Convert to RKNN](#4-convert-to-rknn)
- [5. Python Demo](#5-python-demo)
- [6. Android Demo](#6-android-demo)
- [7. Linux Demo](#7-linux-demo)
- [8. Expected Results](#8-expected-results)



## 1. Description

YOLO26 adopts a **one2one** detection head design, which does not require NMS (Non-Maximum Suppression), making it the main difference from traditional detectors like YOLOv8.
The model used in this example comes from the following open source projects:
https://github.com/ultralytics/ultralytics.git

### Model Characteristics

Since YOLO26 uses a one2one detection head, we adopt a **truncation strategy** when converting the model:
- Post-processing is performed on the host CPU
- The model outputs are truncated into two branches:
  - **Sigmoid branch**: `(1, 80, 8400)` - Class scores (after sigmoid activation)
  - **Mul_2 branch**: `(1, 4, 8400)` - Bounding box coordinates (x1, y1, x2, y2)

### Post-processing

YOLO26's post-processing is very simple, **no NMS is required**:
1. Find the maximum score and corresponding class for each anchor
2. Filter by confidence threshold
3. Sort by score and take TopK



## 2. Current Support Platform

RK3562, RK3566, RK3568, RK3576, RK3588, RV1126B, RV1106, RV1103


## 3. Pretrained Model

### 3.1 Export ONNX Model

You can export the ONNX model from PyTorch pretrained model using the following commands:

```shell
pip install ultralytics
yolo export model=yolo26n.pt format=onnx
```

**Note**: 
- Replace `yolo26n.pt` with other model variants (e.g., `yolo26s.pt`, `yolo26m.pt`) as needed.
- The exported ONNX model will be saved in the same directory as the PyTorch model.

### 3.2 Download Pretrained Model

Download link: 

[./yolo26s.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/95f00b0fc900458ba134f8b180b3f7a1/examples/yolo26/yolo26s.onnx)

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
python convert.py ../model/yolo26s.onnx rk3588
# output model will be saved as ../model/yolo26.rknn
```

*Description:*

- `<onnx_model>`: Specify ONNX model path.
- `<TARGET_PLATFORM>`: Specify NPU platform name. Such as 'rk3588'.
- `<dtype>(optional)`: Specify as `i8` or `fp`. `i8` for doing quantization, `fp` for no quantization. Default is `i8`.
- `<output_rknn_path>(optional)`: Specify save path for the RKNN model, default save in the same directory as ONNX model with name `yolo26.rknn`



## 5. Python Demo

*Usage:*

```shell
cd python
# Inference with ONNX model
python yolo26.py --model_path <onnx_model> --img_show

# Inference with RKNN model
python yolo26.py --model_path <rknn_model> --target <TARGET_PLATFORM> --img_show
```

*Description:*

- `<TARGET_PLATFORM>`: Specify NPU platform name. Such as 'rk3588'.

- `<onnx_model / rknn_model>`: Specify the model path.



## 6. Android Demo

#### 6.1 Compile and Build

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#android-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.  
**Note: Please replace the model name with `yolo26`.**

#### 6.2 Push demo files to device

With device connected via USB port, push demo files to devices:

```shell
adb root
adb remount
adb push install/<TARGET_PLATFORM>_android_<ARCH>/rknn_yolo26_demo/ /data/
```

#### 6.3 Run demo

```sh
adb shell
cd /data/rknn_yolo26_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolo26_demo model/yolo26.rknn model/bus.jpg
```

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```sh
  adb pull /data/rknn_yolo26_demo/out.png
  ```

- Output result refer [Expected Results](#8-expected-results).



## 7. Linux Demo

#### 7.1 Compile and Build

Please refer to the [Compilation_Environment_Setup_Guide](../../docs/Compilation_Environment_Setup_Guide.md#linux-platform) document to setup a cross-compilation environment and complete the compilation of C/C++ Demo.
**Note: Please replace the model name with `yolo26`.**

#### 7.2 Push demo files to device

- If device connected via USB port, push demo files to devices:

```shell
adb push install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolo26_demo/ /userdata/
```

- For other boards, use `scp` or other approaches to push all files under `install/<TARGET_PLATFORM>_linux_<ARCH>/rknn_yolo26_demo/` to `userdata`.

#### 7.3 Run demo

```sh
adb shell
cd /userdata/rknn_yolo26_demo

export LD_LIBRARY_PATH=./lib
./rknn_yolo26_demo model/yolo26.rknn model/bus.jpg
```

- After running, the result was saved as `out.png`. To check the result on host PC, pull back result referring to the following command: 

  ```
  adb pull /userdata/rknn_yolo26_demo/out.png
  ```

- Output result refer [Expected Results](#8-expected-results).



## 8. Expected Results

This example will print the labels and corresponding scores of the test image detect results, as follows:

```
person @ (109 236 226 535) 0.910
bus @ (99 136 555 439) 0.895
person @ (475 232 559 521) 0.879
person @ (212 241 286 508) 0.847
person @ (80 326 123 517) 0.592
```

<img src="out.png">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.
