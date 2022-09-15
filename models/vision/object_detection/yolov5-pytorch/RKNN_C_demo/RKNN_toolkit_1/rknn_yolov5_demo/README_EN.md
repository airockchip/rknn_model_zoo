# Yolov5 C_demo

#### Feature

- Support jpeg,png,bmp picture, resize with pad to keep ratio between width and height.
- Support fp/u8 post-process
- Support single img inference and COCO benchmark
- WARNING: RGA resize function for RGB picture is conditional, the width of picture should be dividable by 4. Otherwise the resize result will get error. For model detail please refer to [RGA doc](https://github.com/rockchip-linux/linux-rga/blob/im2d/docs/RGA_API_Instruction.md)



## Prepare

Refer to [doc](../../../README_EN.md) and export RKNN model. Notice to select the device platform according to your device.



## Build

According to your device, modify 'RK1808_TOOL_CHAIN/RV1109_TOOL_CHAIN' and 'GCC_COMPILER' and execute `./build.sh`

- Notice: rv1109/rv1126 share the same toolchain



## Install

Connect device via adb and push the demo to the device.

```
adb push install/rknn_yolov5_demo /userdata/
```

Push your RKNN model to board. Assume the model name is yolov5_u8.rknn

```
adb push ./yolov5_u8.rknn /userdata/rknn_yolov5_demo/model/yolov5_u8.rknn
```



## Run

```
adb shell
cd /userdata/rknn_yolov5_demo/
./rknn_yolov5_demo ./model/yolov5_u8.rknn u8 single_img ./data/bus.jpg 
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
