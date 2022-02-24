# Yolov5 C_demo
#### 简要说明

- 支持jpeg,png,bmp格式，resize使用补边处理维持宽高比例
- 支持fp/u8后处理
- 支持单图推理及COCO benchmark测试
- 移植时请注意RGA的RGB resize功能需要width尺寸符合4对齐，否则会造成结果异常！更详细的信息请参考[RGA文档](https://github.com/rockchip-linux/linux-rga/blob/im2d/docs/RGA_API_Instruction.md)



## 准备工作

根据[文档教程](../../../README.md)，导出RKNN模型，注意根据手上设备选取正确的平台



## 编译(build)

根据设备，调整RK1808_TOOL_CHAIN/RV1109_TOOL_CHAIN参数路径及GCC_COMPILER参数，执行下面命令进行编译

```
./build.sh
```

- rv1126/rv1109使用同一套编译工具



## 安装(install)

通过adb连接设备并将编译结果推送至板端，执行以下命令推送，这里板端默认使用/userdata路径

```
adb push install/rknn_yolov5_demo /userdata/
```

将前面准备好的RKNN模型推送至板端，这里假设模型名字为yolov5_u8.rknn

```
adb push ./yolov5_u8.rknn /userdata/rknn_yolov5_demo/model/yolov5_u8.rknn
```



## 执行

```
adb shell
cd /userdata/rknn_yolov5_demo/
./rknn_yolov5_demo ./model/yolov5_u8.rknn u8 single_img ./data/bus.jpg 
```



## Demo参数说明

```
./rknn_yolov5_demo <model_path> [fp/u8] [single_img/multi_imgs] <data_path>
```

- <model_path>：指向模型路径
- [fp/u8]：
  - fp 表示使用浮点推理结果进行后处理，适用所有类型的模型
  - u8 表示使用u8推理结果进行后处理，只适用于u8量化模型，处理速度更快，rk1808上约10ms提升
- [single_img/multi_imgs]：
  - single_img 单图推理模式
    - 输入: 图片路径
    - 推理结果：结果保存为out.bmp
  - multi_imgs coco benchmark测试模式
    - 输入：数据集路径txt文件（需将val2017文件夹推送至rknn_yolov5_demo路经下，参考[文档](../../../../../../../datasets/README.md)获取）
    - 推理结果：保存为result_record.txt
    - 获取map值需将result_record.txt文件回传，执行RKNN_python_demo里的[coco_benchmark_with_txt.py](../../../RKNN_python_demo/coco_benchmark_with_txt.py)
    - 如果想要与yolov5仓库的map比较，注意需要将postprocess.h里面的OBJ_NUMB_MAX_SIZE设更大避免部分物体丢失(>6400)；注意main.cc里面的conf_threshold设置，yolov5官方库设为0.001.
- <data_path>：输入路径

