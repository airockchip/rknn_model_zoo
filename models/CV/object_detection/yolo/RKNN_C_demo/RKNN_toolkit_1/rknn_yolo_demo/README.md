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



## 编译(build)

根据设备，调整 RK1808_TOOL_CHAIN/RV1109_TOOL_CHAIN 参数路径及 GCC_COMPILER 参数，执行下面命令进行编译

```
./build.sh
```

- rv1126/rv1109使用同一套编译工具



## 安装(install)

通过adb连接设备并将编译结果推送至板端，执行以下命令推送，这里板端默认使用/userdata路径

```
adb push install/rknn_yolo_demo /userdata/
```

将前面准备好的RKNN模型推送至板端，这里假设模型名字为 yolov5s_u8.rknn

```
adb push ./yolov5s_u8.rknn /userdata/rknn_yolo_demo/model/yolov5s_u8.rknn
```



## 单图测试执行

```
adb shell
cd /userdata/rknn_yolo_demo/
./rknn_yolo_demo yolov5 u8 single_img ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/dog.jpg 
```



## 多图测试

```
cd $(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')
adb push rknn_model_zoo/datasets/COCO/val2017 /userdata/

adb shell
cd /userdata/rknn_yolo_demo/
./rknn_yolo_demo yolov5 u8 multi_imgs ./yolov5s_u8.rknn ./model/RK_anchors_yolov5.txt ./model/coco_dataset_path.txt
```



## Demo参数说明

```
./rknn_yolo_demo [yolov5/yolov7/yolox] [fp/u8] [single_img/multi_imgs] <rknn model path> <anchor file path> <input_path>
```

- 参数 1 为模型类型，根据模型填入 yolov5/ yolov7/ yolox 中的一个
- 参数 2 为后处理类型，u8量化模型可以填入 u8，可有效提升推理速度
- 参数 3 为输入类型，单图测试填 single_img，多图测试填 multi_imgs
- 参数 4 为模型路径，填入模型路径
- 参数 5 为 anchor 文件路径，填入anchor文件路径，模型为yolox类型时，此参数无效，填入任意字符串即可
- 参数 6 为输入路径，单图时填入图片路径，多图时填入 .txt 路径文件



## 与Python demo结果的差异

- 由于 Python demo 使用 opencv 的库对图片进行操作，而 C demo 基于 CImg 、RGA 对图片进行预处理操作，会导致结果存在差异。该差异也会导致Python api/ Capi 的map测试结果差异！
- 若想复现出一样的结果，可以将 src/main.cc 的 DUMP_INPUT 宏设为1，此时会 dump capi 的输入以二进制的格式无损保存为 demo_c_input_hwc_rgb.txt 文件。在 Python demo 中使用 np.fromfile('./input_b/demo_c_input_hwc_rgb.txt', dtype=np.uint8).reshape(640,640,3) 读取该文件即可获取和 C demo一样的输入，此时 Python demo, C demo 可以获得一致的推理结果

