# 模型转换

​	模型支持: Yolov5, Yolov6, Yolov7, Yolox

​	平台支持: [RKNN-Toolkit1](https://github.com/rockchip-linux/rknn-toolkit), [RKNN-Toolki2](https://github.com/rockchip-linux/rknn-toolkit2)



## 使用步骤

这里以测试yolov5s官方下载的权重、rk1808为指定平台为例，注意需要先导出 yolo.torchscript 模型！

- 修改 yml 文件中 model_file_path 参数，**指定模型路径**

- 修改 yml 文件中 RK_device_platform 参数，**指定RKNN平台**

- 默认使用量化，请注意先准备好COCO测试数据集，详见工程目录下datasets内容，如不使用量化，请将 yml 文件中 quantize 参数设为 False

- 默认不启用预编译功能，如需启用请将 yml文件中 pre_compile 参数设为 online，需要连接npu设备（此功能仅在 RKNN-Toolkit1 上有效，RKNN-Toolkit2 没有此功能）

- 如需使用模拟器，请将 yml 文件中 RK_device_id 设为 simulator

  注意：**我们强烈推荐使用连板模式，使用模拟器验证时结果是可能有偏差的**
  
- 如果是自己训练的模型及数据，请将 yml 中 dataset 路径指定到对应的训练/测试数据上，model_file_path指定到对应的pt模型路径，模型输入尺寸由 3,640,640 改为 3,h,w，如 3,736,1280

- 测试 coco benchmark 时，建议使用 200 - 500 张图片进行量化。



#### Yolov5, Yolov6, Yolov7 模型转换指令

```python
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./yolov5_6_7.yml --python_api_test --capi_test
```

或

```
./convert_yolov5_6_7.sh
```

或

```
export rknn_convert=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')/common/rknn_converter/rknn_convert.py
python $rknn_convert --yml_path ./yolov5_6_7.yml --python_api_test --capi_test
```



#### YoloX 模型转换指令

```python
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./yolox.yml --python_api_test --capi_test
```

或

```
./convert_yolox.sh
```

或

```
export rknn_convert=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')/common/rknn_converter/rknn_convert.py
python $rknn_convert --yml_path ./yolox.yml --python_api_test --capi_test
```
