<!--- SPDX-License-Identifier: Apache-2.0 -->

# RKNN模型库
​	Rockchip Neural Network(RKNN)是瑞芯微为了加速模型推理而基于自身NPU硬件架构定义的一套模型格式.使用该格式定义的模型在Rockchip NPU上可以获得远高于CPU/GPU的性能.  
​	RKNN Model Zoo是由像您这样的社区成员贡献的覆盖不同任务,不同框架的SOTA模型的集合.它涵盖了模型转换,模型评估和模型部署等基于Rockchip NPU的完整AI应用开发流程. 每个模型都包含**模型转换,模型评估,模型部署**相关的脚本,如果有对模型原始工程调整的,还会提供相应的脚本或补丁.  

<br>



相关模型权重请从[百度网盘](https://eyun.baidu.com/s/3humTUNq)获取，密码为 rknn

(网盘提供 RKNN 模型如无特别说明，则基于最新 release 版本的 RKNN-Toolkit1/2 生成，使用 RKNN 模型时请先将设备的 NPU 驱动更新至最新的 release 版本；否则需要用户自行生成 RKNN 模型)



## 更新简述

<details>
<summary>&#160&#160&#160 2022-11-15</summary>
模型新增:<br/>
&#160&#160&#160&#160 1.新增 yolov7,yolox 支持<br/>
功能优化:<br/>
&#160&#160&#160&#160 1.RKNN-convert新增 capi test 功能。(beta版本)<br/>
&#160&#160&#160&#160 2.新增定频工具<br/>
</details>




<br>


## 模型概览

### 目标检测
这组模型的作用是识别图像中出现的物体,给出物体所在位置和物体的类别.  

|模型 |说明 |
|-|-|
|<b>[YOLO](models/CV/object_detection/yolo)</b>|支持 yolo 系列的检测模型，目前包含 yolov5/ yolov7/ yolox，支持 RKNN-Toolkit1/2|

<br>

### 语言翻译
实现不同语言的翻译，如中英翻译

|模型 |说明 |
|-|-|
|<b>[Lite transformer](models/NLP/NMT/lite-transformer)</b>|以英文转中文翻译作为示例，支持 RKNN-Toolkit2|



## Acknowledgements

<details>
<summary>&#160&#160&#160 expand</summary>
https://github.com/ultralytics/yolov5<br/>
https://github.com/WongKinYiu/yolov7<br/>
https://github.com/Megvii-BaseDetection/YOLOX<br/>
https://github.com/mit-han-lab/lite-transformer<br/>
</details>
