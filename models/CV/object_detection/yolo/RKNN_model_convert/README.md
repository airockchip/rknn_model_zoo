# 模型转换

​	模型支持: Yolo[v5, v6, v7, v8], ppyoloe_plus, yolox

​	平台支持: [RKNN-Toolkit1](https://github.com/rockchip-linux/rknn-toolkit), [RKNN-Toolki2](https://github.com/rockchip-linux/rknn-toolkit2)



## 使用步骤

#### 1.根据模型修改 yml 配置文件参数

- 必填项

  - model_framework 参数，**指定模型来源框架，如 onnx / pytorch.**
  - model_file_path 参数，**指定模型路径**
  - RK_device_platform 参数，**指定RKNN平台**

- 可选项

  - 默认使用量化。请注意先准备好COCO测试数据集（下载可参考工程目录下datasets内容）。如不使用量化功能，请将 quantize 参数设为 False

  - 默认不启用预编译功能。如需启用请将 pre_compile 参数设为 online，并通过usb口连接npu设备（此功能仅在 RKNN-Toolkit1 上有效，usb口需要能adb连上npu设备，RKNN-Toolkit2 没有此配置）

  - 如需使用模拟器，请将 RK_device_id 设为 simulator

    注意：**我们强烈推荐使用连板模式，使用模拟器验证时结果是可能有偏差的**

  - 如果是自己训练的模型及数据，请将 dataset 路径指定到对应的训练/测试数据上，model_file_path指定到对应的pt模型路径，模型输入尺寸由 3,640,640 改为 3,h,w，如 3,736,1280

  - 测试 coco benchmark 时，建议使用 200 - 500 张图片进行量化。



#### Yolo[v5,v6,v7,v8], ppyoloe_plus 模型转换指令

```python
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./yolo_ppyolo.yml --python_api_test --capi_test
```

或

```
./convert_yolo_ppyolo.sh
```

或

```
export rknn_convert=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')/common/rknn_converter/rknn_convert.py
python $rknn_convert --yml_path ./yolo_ppyolo.yml --python_api_test --capi_test
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
