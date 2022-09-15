<!--- SPDX-License-Identifier: Apache-2.0 -->

# RKNN模型库
​	Rockchip Neural Network(RKNN)是瑞芯微为了加速模型推理而基于自身NPU硬件架构定义的一套模型格式.使用该格式定义的模型在Rockchip NPU上可以获得远高于CPU/GPU的性能.  
​	RKNN Model Zoo是由像您这样的社区成员贡献的覆盖不同任务,不同框架的SOTA模型的集合.它涵盖了模型转换,模型评估和模型部署等基于Rockchip NPU的完整AI应用开发流程. 每个模型都包含**模型转换,模型评估,模型部署**相关的脚本,如果有对模型原始工程调整的,还会提供相应的脚本或补丁.  

## 模型概览

#### 视觉
* [图像分类](#image_classification)
* [图像分割](#image_segmentation)
* [目标检测](#object_detection)
* [人脸分析](#face_analysis)
* [身体分析](#body_analysis)

### 图像分类 <a name="image_classification"/>
这组模型的作用是将图像归类,例如猫,狗,鸟等.  
具体的模型将在后续持续更新.  

### 图像分割 <a name="image_segmentation"/>
这组模型的作用是将图像中的不同物体区分,例如道路图像中,将车/道路/交通灯/建筑等物体区分开来.  
具体的模型将在后续持续更新.  

### 目标检测 <a name="image_segmentation"/>
这组模型的作用是识别图像中出现的物体,给出物体所在位置和物体的类别.  

|模型 |说明 |
|-|-|
|<b>[YOLOv5](models/vision/object_detection/yolov5-pytorch)</b>|YOLOv5是一系列在COCO数据集上预训练的目标检测架构和模型|
<hr>

### 人脸分析 <a name="face_analysis"/>
这组模型的作用是检测图像中出现的人脸,并标记人脸所在的位置.  
具体的模型将在后续持续更新.

### 身体分析 <a name="body_analysis"/>
这组模型的作用是检测人体姿态,手势等.  
具体的模型将在后续持续更新.  
