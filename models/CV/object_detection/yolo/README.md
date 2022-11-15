# YOLO - demo

## 内容说明

​	针对 RKNN 平台，本仓库提供了yolo系列模型的使用教程，包含模型转换、精度测试、C++ Demo。提供部分 YOLO 模型在 RKNN 平台上的性能、精度测试结果。


​	仓库使用的 yolo 模型，针对 RKNN 平台对模型做了一些结构适配优化，适配代码提交在原始仓库的fork分支上 (相比之前的版本，现在把**模型结构尾部的 sigmoid 函数整合进模型里面**，理论上可以获得精度更高的量化效果。而之前的处理是丢掉最后的 sigmoid 函数放在后处理上进行，这个**差异会导致模型、测试脚本与之前的不兼容，使用时请留意**）。

<br>

​	支持简表:

<table>
    <tr>
        <td></td>
        <td>Yolov5</td>
        <td>Yolov7</td>
        <td>YOLOX</td>
    </tr>
    <tr>
        <td>Toolkit1 - python demo</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>&#160&#160&#160&#160 &#160&#160&#160&#160 &#160&#160&#160&#160 &#160- C demo</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>Toolkit2 - python demo</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>&#160&#160&#160&#160 &#160&#160&#160&#160 &#160&#160&#160&#160 &#160- C demo</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
        <td>&#10004;</td>
    </tr>
</table>

<br>

## 使用步骤

### 	步骤1. 获取模型		

​		使用 [airockchip](https://github.com/airockchip) 组织下的 [yolov5](https://github.com/airockchip/yolov5)/ [yolov7](https://github.com/airockchip/yolov7)/ [YOLOX](https://github.com/airockchip/YOLOX) 仓库，根据仓库里的 README_rkopt_manual.md 提示，进行模型训练、模型导出。获取 torchscript / onnx 格式的模型。

​		或下载已经提供的RKNN模型，获取[网盘(密码:rknn)](https://eyun.baidu.com/s/3humTUNq) ，链接中的具体位置为rknn_model_zoo/models/CV/object_detection/yolo/{YOLOX/yolov7/yolov5}/deploy_models/{toolkit}/{platform}

<br>

### 	步骤2. 模型转换

​		参考 [RKNN_model_convert](./RKNN_model_convert/README.md) 说明，将步骤1获取的 torchscript / onnx 格式的模型转为 RKNN 模型。

​		在连板调试功能可以正常使用的情况下，转换脚本除了转出模型，会调用  rknn.eval_perf 接口获取 RKNN 模型推理性能、计算 RKNN 模型与原模型的推理结果相似度(基于cos距离)。

<br>

### 	步骤3. 使用 Python demo 测试 RKNN/ torchscript/ onnx模型

​		参考 [RKNN_python_demo](./RKNN_python_demo/README.md) 说明，测试 RKNN/ torchscript/ onnx 格式的 yolo 模型在图片上推理结果并绘图。

- 支持指定文件夹测试，测试该文件夹下的所有图片
- 支持 COCO benchmark 测试

<br>

### 	步骤4. 使用 C demo 测试 RKNN 模型

​		RK1808/ RV1109/ RV1126 请使用 [RKNN_toolkit_1](./RKNN_C_demo/RKNN_toolkit_1/rknn_yolo_demo) 版本

​		RK3566/ RK3568/ RK3588 请使用 [RKNN_toolkit_2](./RKNN_C_demo/RKNN_toolkit_2/rknn_yolo_demo) 版本

- 支持单图测试
- 支持 COCO benchmark 测试
- 支持对 u8/fp 输出进行后处理

<br>

<br>


## 附录：

### 前后处理差异

- 不同的 YOLO 模型存在前后处理差异，需要注意这些差异避免预测结果与原始框架不一致。

<details>
<summary>&#160&#160&#160差异详情</summary>
<table>
    <tr>
        <td></td>
        <td>Yolov5</td>
        <td>Yolov7</td>
        <td>Yolox</td>
    </tr>
    <tr>
        <td>Color</td>
        <td>RGB</td>
        <td>RGB</td>
        <td>BGR</td>
    </tr>
    <tr>
        <td>Mean</td>
        <td>0</td>
        <td>0</td>
        <td>0</td>
    </tr>
    <tr>
        <td>Std</td>
        <td>255</td>
        <td>255</td>
        <td>1</td>
    </tr>
    <tr>
        <td>Letter_box pad_color</td>
        <td>(114,114,114)</td>
        <td>(114,114,114)</td>
        <td>(114,114,114)</td>
    </tr>
    <tr>
        <td>NMS - class agnostic</td>
        <td>False</td>
        <td>False</td>
        <td>Yes</td>
    </tr>
    <tr>
        <td>conf threshold</td>
        <td>0.25@box<br>0.25@(box* class)</td>
        <td>0.25@box<br>0.25@(box* class)</td>
        <td>0.25@(box* class)</td>
    </tr>
    <tr>
        <td>nms threshold</td>
        <td>0.45</td>
        <td>0.45</td>
        <td>0.45</td>
    </tr>
    <tr>
        <td>Anchors</td>
        <td>Yes</td>
        <td>Yes</td>
        <td>No</td>
    </tr>
</table>
*这些差异实现已经整合进本仓库的demo及脚本里
</details>

<br>

### MAP精度测试

|                         | Val type                   | Pytorch - float | RKNN-Toolkit1 - u8<br> (per layer) | RKNN-Toolkit2 - i8<br> (per channel) |
| ----------------------- | -------------------------- | --------------- | ---------------------------------- | ------------------------------------ |
| Yolov5s - relu          | mAP<sup>val</sup> 0.5:0.95 | 0.3530          | 0.3385                             | 0.3475                               |
|                         | mAP<sup>val</sup> 0.5      | 0.5467          | 0.5365                             | 0.5449                               |
| Yolov5s - silu          | mAP<sup>val</sup> 0.5:0.95 | 0.3700          | 0.3498                             | 0.3526                               |
|                         | mAP<sup>val</sup> 0.5      | 0.5615          | 0.5485                             | 0.5466                               |
| Yolov7-tiny - leakyrelu | mAP<sup>val</sup> 0.5:0.95 | 0.3685          | 0.3497                             | 0.3509                               |
|                         | mAP<sup>val</sup> 0.5      | 0.5452          | 0.5325                             | 0.5295                               |
| Yoloxs - silu           | mAP<sup>val</sup> 0.5:0.95 | 0.3908          | 0.3548                             | 0.3729                               |
|                         | mAP<sup>val</sup> 0.5      | 0.5754          | 0.5506                             | 0.5600                               |

<details>
<summary>&#160&#160&#160测试环境</summary>
&#160&bull;&#160 基于 RKNN_python_demo 的 yolo_map_test_rknn.py 脚本获取测试结果，测试采用固定输入shape，测试结果会比动态shape差一点 <br/>
&#160&bull;&#160 RV1126/RV1109/RK1808/RK3399pro 测试基于 1.7.3 版本的 RKNPU、RKNN-Toolkit <br/>
&#160&bull;&#160 RK3566/RK3568/RK3588 测试基于 1.4.0 版本的 RKNPU2、RKNN-Toolkit2 <br/>
&#160&bull;&#160 如果用户使用自己的模型，测试发现map掉精度比较严重，可以尝试使用 Pytorch 的 QAT 量化 <br/>
</details>

<details>
<summary>&#160&#160&#160权重说明</summary>
&#160&bull;&#160 除 yolov5s-relu，其他均为对应官方仓库发布的预训练权重<br/>
&#160&bull;&#160 权重可从百度网盘获取: https://eyun.baidu.com/s/3humTUNq  密码: rknn <br/>
</details>

<br>

### 推理性能(8 Bit量化)

<details open>
<summary>&#160&#160&#160每秒帧率(fps),数值越大性能越强</summary>
<table>
<tr>
    <td>\Platform<br>\Result(fps)<br>Model</td>
    <td>RV1109</td>
    <td>RV1126</td>
    <td>RV1808</td>
    <td>RK3399pro</td>
    <td>RK3566</td>
    <td>RK3568</td>
    <td>RK3588<br>single core</td>
</tr>
<tr>
    <td>Yolov5s - relu</td>
    <td>20.3</td>
    <td>29.4</td>
    <td>37.0</td>
    <td>37.7</td>
    <td>23.5</td>
    <td>24.6</td>
    <td>53.8</td>
</tr>
<tr>
    <td>Yolov5s - silu</td>
    <td>13.6</td>
    <td>20.1</td>
    <td>28.1</td>
    <td>28.5</td>
    <td>14.0</td>
    <td>14.7</td>
    <td>24.2</td>
</tr>
<tr>
    <td>Yolov5m - relu</td>
    <td>8.3</td>
    <td>12.1</td>
    <td>16.9</td>
    <td>17.2</td>
    <td>10.4</td>
    <td>10.8</td>
    <td>21.7</td>
</tr>
<tr>
    <td>Yolov5m - silu</td>
    <td>5.7</td>
    <td>8.4</td>
    <td>13.0</td>
    <td>13.2</td>
    <td>6.5</td>
    <td>6.7</td>
    <td>11.0</td>
</tr>
<tr>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
</tr>
<tr>
    <td>Yolov7tiny - relu</td>
    <td>27.8</td>
    <td>40.1</td>
    <td>46.3</td>
    <td>47.4</td>
    <td>27.3</td>
    <td>30.6</td>
    <td>67.1</td>
</tr>
<tr>
    <td>Yolov7tiny - leakyrelu</td>
    <td>15.7</td>
    <td>22.8</td>
    <td>37.3</td>
    <td>37.9</td>
    <td>27.4</td>
    <td>30.6</td>
    <td>66.8</td>
</tr>
<tr>
    <td>Yolov7 - relu</td>
    <td>4.6</td>
    <td>6.7</td>
    <td>9.9</td>
    <td>10.1</td>
    <td>5.3</td>
    <td>5.6</td>
    <td>12.5</td>
</tr>
<tr>
    <td>Yolov7 - silu</td>
    <td>3.3</td>
    <td>4.9</td>
    <td>7.4</td>
    <td>7.5</td>
    <td>3.5</td>
    <td>3.7</td>
    <td>6.9</td>
</tr>
<tr>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
</tr>
<tr>
    <td>YoloXs - relu</td>
    <td>15.8</td>
    <td>22.8</td>
    <td>30.5</td>
    <td>30.9</td>
    <td>19.8</td>
    <td>21.0</td>
    <td>44.7</td>
</tr>
<tr>
    <td>YoloXs - silu</td>
    <td>10.6</td>
    <td>15.7</td>
    <td>22.9</td>
    <td>23.1</td>
    <td>11.6</td>
    <td>12.2</td>
    <td>20.0</td>
</tr>
<tr>
    <td>YoloXm - relu</td>
    <td>6.5</td>
    <td>9.5</td>
    <td>13.4</td>
    <td>13.6</td>
    <td>8.4</td>
    <td>8.8</td>
    <td>18.2</td>
</tr>
<tr>
    <td>YoloXm - silu</td>
    <td>4.6</td>
    <td>6.7</td>
    <td>10.5</td>
    <td>10.6</td>
    <td>5.3</td>
    <td>5.5</td>
    <td>9.0</td>
</tr>
    </table></details>

<details>
<summary>&#160&#160&#160单帧推理耗时(单位:ms)</summary>
<table>
    <tr>
        <td>\Platform<br>\Result(ms)<br>Model</td>
        <td>RV1109</td>
        <td>RV1126</td>
        <td>RV1808</td>
        <td>RK3399pro</td>
        <td>RK3566</td>
        <td>RK3568</td>
        <td>RK3588<br>single core</td>
    </tr>
    <tr>
        <td>Yolov5s - relu</td>
        <td>49.35</td>
        <td>34.04</td>
        <td>27.02</td>
        <td>26.56</td>
        <td>42.58</td>
        <td>40.64</td>
        <td>18.60</td>
    </tr>
    <tr>
        <td>Yolov5s - silu</td>
        <td>73.36</td>
        <td>49.87</td>
        <td>35.57</td>
        <td>35.10</td>
        <td>71.36</td>
        <td>68.12</td>
        <td>41.31</td>
    </tr>
    <tr>
        <td>Yolov5m - relu</td>
        <td>120.83</td>
        <td>82.35</td>
        <td>59.04</td>
        <td>58.11</td>
        <td>95.89</td>
        <td>92.66</td>
        <td>46.02</td>
    </tr>
    <tr>
        <td>Yolov5m - silu</td>
        <td>176.07</td>
        <td>119.45</td>
        <td>77.01</td>
        <td>75.96</td>
        <td>153.48</td>
        <td>148.38</td>
        <td>90.54</td>
    </tr>
    <tr>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
    </tr>
    <tr>
        <td>Yolov7tiny - relu</td>
        <td>36.01</td>
        <td>24.94</td>
        <td>21.6</td>
        <td>21.08</td>
        <td>36.61</td>
        <td>32.64</td>
        <td>14.91</td>
    </tr>
    <tr>
        <td>Yolov7tiny - leakyrelu</td>
        <td>63.54</td>
        <td>43.85</td>
        <td>26.79</td>
        <td>26.37</td>
        <td>36.51</td>
        <td>32.64</td>
        <td>14.97</td>
    </tr>
    <tr>
        <td>Yolov7 - relu</td>
        <td>218.44</td>
        <td>148.81</td>
        <td>100.80</td>
        <td>99.08</td>
        <td>188.85</td>
        <td>179.98</td>
        <td>79.76</td>
    </tr>
    <tr>
        <td>Yolov7 - silu</td>
        <td>303.15</td>
        <td>205.37</td>
        <td>135.78</td>
        <td>134.08</td>
        <td>282.58</td>
        <td>269.54</td>
        <td>144.85</td>
    </tr>
    <tr>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
        <td></td>
    </tr>
    <tr>
        <td>YoloXs - relu</td>
        <td>63.36</td>
        <td>43.77</td>
        <td>32.84</td>
        <td>32.40</td>
        <td>50.54</td>
        <td>47.70</td>
        <td>22.37</td>
    </tr>
    <tr>
        <td>YoloXs - silu</td>
        <td>94.45</td>
        <td>63.55</td>
        <td>43.68</td>
        <td>43.26</td>
        <td>86.26</td>
        <td>81.91</td>
        <td>50.06</td>
    </tr>
    <tr>
        <td>YoloXm - relu</td>
        <td>153.09</td>
        <td>104.87</td>
        <td>74.54</td>
        <td>73.42</td>
        <td>119.00</td>
        <td>113.75</td>
        <td>54.84</td>
    </tr>
    <tr>
        <td>YoloXm - silu</td>
        <td>219.11</td>
        <td>149.24</td>
        <td>95.66</td>
        <td>94.58</td>
        <td>189.76</td>
        <td>181.78</td>
        <td>110.57</td>
    </tr>
</table>
</details>

<details>
<summary>&#160&#160&#160测试环境</summary>
&#160&bull;&#160 基于 RKNN_model_convert 的模型转换脚本获取测试结果 <br/>
&#160&bull;&#160 RV1126/RV1109/RK1808/RK3399pro 测试基于 1.7.3 版本的 RKNPU、RKNN-Toolkit <br/>
&#160&bull;&#160 RK3566/RK3568/RK3588 测试基于 1.4.0 版本的 RKNPU2、RKNN-Toolkit2 <br/>
</details>

<details>
<summary>&#160&#160&#160频率设置</summary>
<table>
    <tr>
        <td>Platfrom</td>
        <td>CPU freq(GHz)</td>
        <td>DDr freq(GHz)</td>
        <td>NPU freq(GHz)</td>
    </tr>
    <tr>
        <td>RV1109</td>
        <td>1.512</td>
        <td>0.934</td>
        <td>0.594</td>
    </tr>
    <tr>
        <td>RV1126</td>
        <td>1.512</td>
        <td>0.934</td>
        <td>0.934</td>
    </tr>
    <tr>
        <td>RK1808</td>
        <td>1.512</td>
        <td>0.934</td>
        <td>0.792</td>
    </tr>
    <tr>
        <td>RK3399pro</td>
        <td>1.512</td>
        <td>0.934</td>
        <td>0.792</td>
    </tr>
    <tr>
        <td>RK3566</td>
        <td>1.800</td>
        <td>1.056</td>
        <td>1.000</td>
    </tr>
    <tr>
        <td>RK3568</td>
        <td>1.800</td>
        <td>1.560</td>
        <td>1.000</td>
    </tr>
    <tr>
        <td>RK3588(single core)</td>
        <td>2.400</td>
        <td>2.112</td>
        <td>1.000</td>
    </tr>
</table>
</details>


