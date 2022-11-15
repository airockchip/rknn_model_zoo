# Yolo C_demo


#### 简要说明

- 支持 Yolov5, Yolov7, YOLOX
- 支持 jpeg,png,bmp 格式，resize使用补边处理(letter box)维持宽高比例
- 支持 fp/u8 后处理
- 支持单图推理及 COCO benchmark 测试
- 移植时请注意 RGA 的 RGB resize 功能需要 width 尺寸符合4对齐，否则会造成结果异常！更详细的信息请参考[RGA文档](https://github.com/rockchip-linux/linux-rga/blob/im2d/docs/RGA_API_Instruction.md)
- 请注意，该版本使用的 yolo 模型包含尾部的sigmoid op，旧版本不包含sigmoid，使用请勿混用，混用会导致结果异常。




## 准备工作

根据[文档教程](../../../README.md)，根据手上设备选取正确的平台，导出RKNN模型，或下载已经提供的RKNN模型，获取[网盘(密码:rknn)](https://eyun.baidu.com/s/3humTUNq) ，链接中的具体位置为rknn_model_zoo/models/CV/object_detection/yolo/{YOLOX/yolov7/yolov5}/deploy_models/{toolkit}/{platform}

初始化 [RKNN 依赖库](../../../../../../../libs/rklibs)




## Android Demo

### 编译

根据指定平台修改 `build-android_<TARGET_PLATFORM>.sh`中的Android NDK的路径 `ANDROID_NDK_PATH`，<TARGET_PLATFORM>可以是RK356X或RK3588 例如修改成：

```sh
ANDROID_NDK_PATH=~/opt/tool_chain/android-ndk-r17
```

然后执行：

```sh
./build-android_<TARGET_PLATFORM>.sh
```



### 推送执行文件到板子

连接板子的usb口到PC,将整个demo目录到 `/data`:

```sh
adb root
adb remount
adb push install/rknn_yolo_demo_Android /data/
```



### 运行(单图测试)

```sh
adb shell
cd /data/rknn_yolo_demo_Android/

export LD_LIBRARY_PATH=./lib
./rknn_yolo_demo yolov5 q8 single_img ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/dog.jpg 
```





## Aarch64 Linux Demo

### 编译

根据指定平台修改 `build-linux_<TARGET_PLATFORM>.sh`中的交叉编译器所在目录的路径 `TOOL_CHAIN`，例如修改成

```sh
export TOOL_CHAIN=~/opt/tool_chain/gcc-9.3.0-x86_64_aarch64-linux-gnu/host
```

然后执行：

```sh
./build-linux_<TARGET_PLATFORM>.sh
```



### 推送执行文件到板子


将 install/rknn_yolo_demo_Linux 拷贝到板子的/data/目录.


```
adb push install/rknn_yolo_demo_Linux /data/
```



### 运行(单图测试)

```sh
adb shell
cd /data/rknn_yolo_demo_Linux/

export LD_LIBRARY_PATH=./lib

./rknn_yolo_demo yolov5 q8 single_img ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/dog.jpg 
```



## 多图测试

```
cd $(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')
adb push rknn_model_zoo/datasets/COCO/val2017 /userdata/

adb shell
cd /userdata/rknn_yolo_demo/
./rknn_yolo_demo yolov5 q8 multi_imgs ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/coco_dataset_path.txt
```

## Zero Copy Demo
新增zero copy的demo，主要使用以'rknn_create_mem_from_fd'为例子，配合外部分配的drm buffer配置给NPU使用。

**注意：目前的zero copy仅支持Int8模型，不支持浮点模型**

```sh
adb shell
cd /data/rknn_yolo_demo_Android/

export LD_LIBRARY_PATH=./lib

./rknn_yolo_zeroCopy_demo yolox q8 single_img yolox_rk356x_u8.rknn xxx ./model/bus.jpg
```




## Demo参数说明

```
./rknn_yolo_demo [yolov5/yolov7/yolox] [fp/q8] [single_img/multi_imgs] <rknn model path> <anchor file path> <input_path>
```

- 参数 1 为模型类型，根据模型填入 yolov5/ yolov7/ yolox 中的一个
- 参数 2 为后处理类型，q8量化模型可以填入 q8，可有效提升推理速度
- 参数 3 为输入类型，单图测试填 single_img，多图测试填 multi_imgs
- 参数 4 为模型路径，填入模型路径
- 参数 5 为 anchor 文件路径，填入anchor文件路径，模型为yolox类型时，此参数无效，填入任意字符串即可
- 参数 6 为输入路径，单图时填入图片路径，多图时填入 .txt 路径文件



### 注意
- 量化模型输入选项改成q8，而不是u8!

- **注意如果是在3588上使用时，需要将PLATFORM_RK3588的宏定义打开!!! 主要是为了配合RGA的对齐**
