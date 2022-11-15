# Yolov5 C_demo

#### Feature

- Support jpeg,png,bmp picture, resize with pad to keep ratio between width and height.
- Support fp/u8 post-process
- Support single img inference and COCO benchmark
- WARNING: RGA resize function for RGB picture is conditional, the width of picture should be dividable by 4. Otherwise the resize result will get error. For model detail please refer to [RGA doc](https://github.com/rockchip-linux/linux-rga/blob/im2d/docs/RGA_API_Instruction.md)



## Preparation

Refer to [doc](../../../README_EN.md) and export RKNN model. Notice to select the device platform according to your device.


## Android Demo

### Building

According to your device, modify the path in Android NDK in  'build-android_<TARGET_PLATFORM>.sh', where <TARGET_PLATFORM> can be RK356X or RK3588, for example:

```sh
ANDROID_NDK_PATH=~/opt/tool_chain/android-ndk-r17
```
then, execute:

```sh
./build-android_<TARGET_PLATFORM>.sh
```

### Push to device 
Connecting the board to PC via the usb, and push whole demo to `/data`:

```sh
adb root
adb remount
adb push install/rknn_yolo_demo_Android /data/
```

### Run the testing of single image demo 

```sh
adb shell
cd /data/rknn_yolo_demo_Android/

export LD_LIBRARY_PATH=./lib
./rknn_yolo_demo yolov5 q8 single_img ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/dog.jpg 
```


## Aarch64 Linux Demo

### Building
Modifying the TARGET_PLATFORM according to platform and the path of cross compiler via `TOOL_CHAIN` in `build-linux_<TARGET_PLATFORM>.sh`, as following example: 

```sh
export TOOL_CHAIN=~/opt/tool_chain/gcc-9.3.0-x86_64_aarch64-linux-gnu/host
```

then executing,
```sh
./build-linux_<TARGET_PLATFORM>.sh
```

### Push to device
push install/rknn_yolo_demo_Linux to /data/ in device,

```sh
adb push install/rknn_yolo_demo_Linux /data/
```

### Run the demo (single image)
```sh
adb shell
cd /data/rknn_yolo_demo_Linux/

export LD_LIBRARY_PATH=./lib

./rknn_yolo_demo yolov5 q8 single_img ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/dog.jpg 
```


## Demo argument

```
./rknn_yolov5_demo <model_path> [fp/u8] [single_img/multi_imgs] <data_path>
```

- <model_path> : path to the model
- [fp/u8] :
  - fp mean using float result for post_process, fitting for all type model.
  - u8 mean using u8 result for post_process, only fitting for u8 model. Always got better inference speed. On rk1808, the promotion is about 10ms.
- [single_img/multi_imgs]：
  - single_img
    - input: picture path
    - result: saved as 'out.bmp'
  - multi_imgs(coco benchmark test)
    - input: dataset paths txt file. (need to push val2017 folder to 'rknn_yolov5_demo' folder first, refer to [doc](../../../../../../../datasets/README.md) to get the dataset.)
    - result: saved as result_record.txt
    - to get map val test, pull back result_record.txt file and execute [coco_benchmark_with_txt.py](../../../RKNN_python_demo/coco_benchmark_with_txt.py) in RKNN_python_demo folder.
    - If you want to compare map with yolov5 official repository, set OBJ_NUMB_MAX_SIZE in <postprocess.h> to 6400; set conf_threshold in <main.cc> to 0.001
- <data_path>：input path.
