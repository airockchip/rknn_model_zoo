# Yolov5 - pytorch

- 官方仓库地址：https://github.com/ultralytics/yolov5.git

- 默认支持commit_id 为 c9a46a6，**目前优化已经适配 v5, v6 结构**



## 下载仓库并打上patch：

​	在该 README.md 文件所在目录下执行以下指令，

```sh
git clone https://github.com/ultralytics/yolov5.git
cd yolov5
git checkout c9a46a60e09ab94009754ca71bde23e91aab33fe
git apply ../patchs/rknpu_optimize.patch

#对于rknpu设备，我们推荐打上下面的patch，将silu激活层替换成relu，以获得更佳的推理性能。请注意替换后权重需要重新训练！
git apply ../patchs/silu_to_relu.patch
```



---

### 模型训练：

​	请参考官方仓库文档训练即可

​	COCO 数据集下载地址： https://cocodataset.org/#download

注意：

- 使用Silu激活函数训练的模型，与使用ReLU激活函数训练的模型，两者的权重是不一样的，不可以混淆使用！官方权重是使用Silu进行训练！
- 除了 ReLU 替换成 Silu，其他的优化操作仅改变部分算子的结构，且该优化操作不会影响模型的预测结果，故关于如何训练模型，参考官方文档即可，这里不再赘述。
- 对于不同的数据集，yolov5仓库可能会自动求出最佳anchors，导出模型时请留意anchors信息，与后续demo的anchors信息不一致时，需要修改demo中的anchors信息。
- 通常而言，Detect层的stride信息固定为[8,16,32]，如果该定义被修改了，也需要在demo中修改对应信息。



---

### 导出模型及转换模型：

​	在yolov5 目录下执行以下命令，即可导出针对npu优化的模型，同时打印并将anchors保存成txt文件。

```python
python export.py --rknpu {device_platform}
```

- device platform 替换成手上板子对应的平台，有以下选择 [rk1808/rv1109/rv1126/rk3399pro/rk3566/rk3568/rk3588]
- 不同平台具体优化细节略有差异



---

### 转换模型：

请参考 [./RKNN_model_convert/README.md](./RKNN_model_convert/README.md) 文档



---

### Python_demo

请参考 [./RKNN_python_demo/README.md](./RKNN_python_demo/README.md) 文档




---

### C_demo

RKNN_toolkit1 请参考[./RKNN_C_demo/RKNN_toolkit_1/rknn_yolov5_demo/README.md](./RKNN_C_demo/RKNN_toolkit_1/rknn_yolov5_demo/README.md) 文档

RKNN_toolkit2 TO DO




---

### Patch优化细节：

- 大尺寸 Maxpool 优化：大尺寸的 Maxpool 对底层不友好，会导致耗时增加，而从公式上 5x5 Maxpool 与两个串联的 3x3 Maxpool 是等价的，以此类推，我们对 SPP 层 5x5, 9x9, 13x13 的Maxpool 做了分解优化，使模型更适合在 RKNN 设备上部署，且不影响精度。
- Focus层使用卷积优化：slice算子目前对部分 RKNN 设备不友好，我们使用特殊权重分布的卷积替换它，在保证计算结果不变的情况下，优化模型部署速度。
- 去掉后处理的原因：目前大部分模型的后处理对 NPU 设备都不友好，部分后处理op量化后会有精度问题，故我们默认将 yolov5 的后处理结构移除。



---

### 性能测试：

| platform（fps）          | yolov5s-relu | yolov5s-silu | yolov5m-relu | yolov5m-silu |
| ------------------------ | ------------ | ------------ | ------------ | ------------ |
| rk1808 - u8              | 35.24        | 26.41        | 16.27        | 12.60        |
| rv1109 - u8              | 19.58        | 13.33        | 8.11         | 5.45         |
| rv1126 - u8              | 27.54        | 19.29        | 11.69        | 7.86         |
| rk3566 - u8              | 15.16        | 10.60        | 8.65         | 6.61         |
| rk3568 - u8              |              |              |              |              |
| rk3588 - u8(single core) | 53.73        | 33.24        | 22.31        | 14.74        |
| rk3588 - u8(triple core) |              |              |              |              |

具体细节请参考[文档](./RKNN_model_convert/README.md)

