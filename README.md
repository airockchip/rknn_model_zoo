<!--- SPDX-License-Identifier: Apache-2.0 -->

# RKNN模型库
​	Rockchip Neural Network(RKNN)是瑞芯微为了加速模型推理而基于自身NPU硬件架构定义的一套模型格式.使用该格式定义的模型在Rockchip NPU上可以获得远高于CPU/GPU的性能.  
​	RKNN Model Zoo是由像您这样的社区成员贡献的覆盖不同任务、不同应用场景的SOTA模型集合。它涵盖了模型转换、模型评估和模型部署等基于Rockchip NPU的完整AI应用开发流程。

<br/>

除了从提供的github仓库直接导出模型，模型文件也可以从[百度网盘](https://eyun.baidu.com/s/3humTUNq)获取，密码为 rknn

(网盘提供的 RKNN 模型如无特别说明，则基于最新 release 版本的 RKNN-Toolkit1/2 生成，使用 RKNN 模型时请先将设备的 NPU 驱动更新至最新的 release 版本；如无法更新驱动，则需要使用旧版本 RKNN-Toolkit 生成与驱动匹配的 RKNN 模型)

<br/>

为了规范转模型操作，RKNN Model Zoo提供了模型转换工具，通过配置 yaml 的形式转模型，兼容 toolkit1/2，支持对导出的模型进行(包括连板/Capi)验证，包含验证推理精度、验证推理性能、记录测试结果。更多说明请参考[文档](./common/rknn_converter/README.md)



## 更新简述
<details>
<summary>&#160&#160&#160 2023-09-18</summary>
模型新增:<br/>
&#160&#160&#160&#160 1.Yolo 系列模型新增 yolov6,yolov8,ppyoloe 支持<br/>
功能优化:<br/>
&#160&#160&#160&#160 1.完善模型转换、定频功能。新增rv1106/rk3562支持。
</details>


<details>
<summary>&#160&#160&#160 2022-11-15</summary>
模型新增:<br/>
&#160&#160&#160&#160 1.新增 yolov7,yolox 支持<br/>
&#160&#160&#160&#160 2.新增 lite-transformer 支持。(beta版本)<br/>
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
|<b>[YOLO](models/CV/object_detection/yolo)</b>|支持 yolo 系列的检测模型，目前包含 yolov5/6/7/8、ppyoloe、yolox。支持 RKNN-Toolkit1/2|

<br>

### 语言翻译
实现不同语言的翻译，如中英翻译

|模型 |说明 |
|-|-|
|<b>[Lite transformer](models/NLP/NMT/lite-transformer)</b>|以英文转中文翻译作为示例，支持 RKNN-Toolkit1/2|





## 添加模型指南

Fork该仓库并更新代码，提交 merge request ，会有RKNPU的研发人员进行评审、提供意见与帮助、接收代码。代码作者也将添加在致谢/共同作者中。

在 models 目录下找到对应模型的类别(如果对类别不确定，可联系RKNPU研发人员探讨)，在文档中说明模型的开源库来源，并添加对应的模型转换/Python demo/Cdemo。这里推荐基于官方的开源模型库 fork 并使用固定的 commit 进行后续的开发，避免因开源库的更新导致 RKNN Model Zoo 的代码失效。

由于开发者通常只有单一型号的开发板，添加代码时以您手上的板子进行验证即可。代码接收后，RKNPU研发人员会补充并完善其他芯片的兼容性问题。

如遇到任何疑问、问题，欢迎联系 RKNPU研发人员解决。





## Acknowledgements

https://github.com/ultralytics/yolov5<br/>https://github.com/meituan/YOLOv6<br/>https://github.com/WongKinYiu/yolov7<br/>https://github.com/ultralytics/ultralytics<br/>
https://github.com/Megvii-BaseDetection/YOLOX<br/>https://github.com/PaddlePaddle/PaddleDetection<br/>
https://github.com/mit-han-lab/lite-transformer<br/>

