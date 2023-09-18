# Yolo C_demo


#### 简要说明

- 模型支持: Yolo[v5, v6, v7, v8], ppyoloe_plus, yolox
- 支持 jpeg,png,bmp 格式，resize使用补边处理(letter box)维持宽高比例
- 支持 fp/u8 后处理
- 支持单图推理
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
./rknn_yolo_demo yolov5 q8 ./yolov5s_u8.rknn ./model/dog.jpg 
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

./rknn_yolo_demo yolov5 q8 ./yolov5s_u8.rknn ./model/dog.jpg 
```




## Demo参数说明

```
./rknn_yolo_demo [yolov5/6/7/8, ppyoloe_plus, yolox] [fp/q8] <rknn model path>  <input_path>
```

- 参数 1 为模型类型，根据模型填入 yolov5/6/7/8, ppyoloe_plus, yolox 中的一个
- 参数 2 为后处理类型，q8量化模型可以填入 q8，可有效提升推理速度
- 参数 3 为模型路径，填入模型路径
- 参数 4 为输入路径，单图时填入图片路径

