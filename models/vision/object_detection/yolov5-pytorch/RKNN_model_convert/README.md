## 模型转换：

这里以测试yolov5s官方下载的权重、rk1808为指定平台为例，注意需要先导出yolov5s.torchscript模型！

- 这里默认RK平台为RK1808，其他平台请修改yml文件中 RK_device_platform参数

- 默认使用量化，请注意先准备好COCO测试数据集，详见工程目录下datasets内容，如不使用量化，请将 yml文件中 quantize 参数设为 False

- 默认不启用预编译功能，如需启用请将 yml文件中 pre_compile 参数设为 online，需要连接npu设备。

- 如需使用模拟器，请将yml文件中RK_device_id设为simulator。

  注意：我们强烈推荐使用连板模式，使用模拟器验证时结果是可能有偏差的。
  
- 如果是自己训练的模型及数据，请将 yolov5s_convert.yml 中 dataset 路径指定到对应的训练/测试数据上，model_file_path指定到对应的pt模型路径，模型输入尺寸由 3,640,640 改为 3,h,w，如 3,736,1280



在该文档所在目录下执行指令导出RKNN模型：

```python
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./yolov5s_convert.yml --compute_convert_loss --eval_perf
```

或

```
./convert.sh
```



## 速度评估（仅供参考）

使用**rknn.eval**接口评估速度

- 模型输入：1x3x640x640
- RKNN_toolkit1-1.7.1 (For rk3399pro, rk1808, rv1109, rv1126)
- RKNN_toolkit2-1.2.0 (For rk3566, rk3568, rk3588)
- 硬件信息
  - rk3399pro
  - rk1808
    - 型号: RK_EVB_RK1808_LP3D
    - 驱动: 1.7.1
  - rv1109:
    - 型号: RV1109 DDR3 EVB
    - 驱动: 1.7.1
    - 其他: 关闭ISP demo 避免资源占用影响
  - rv1126
    - 型号: RV1126 DDR3 EVB
    - 驱动: 1.7.1
    - 其他: 关闭ISP demo 避免资源占用影响
  - rk3566
    - 型号: RK_EVB_RK3566_LP4XD200P132SD6_V11
    - 驱动: 1.2.0
  - rk3568
  - rk3588
    - 型号: RK_EVB2_RK3588_LP4XD
    - 驱动: 1.2.0

| platform（fps）          | yolov5s-relu | yolov5s-silu | yolov5m-relu | yolov5m-silu |
| ------------------------ | ------------ | ------------ | ------------ | ------------ |
| rk1808 - u8              | 35.24        | 26.41        | 16.27        | 12.60        |
| rv1109 - u8              | 19.58        | 13.33        | 8.11         | 5.45         |
| rv1126 - u8              | 27.54        | 19.29        | 11.69        | 7.86         |
| rk3566 - u8              | 15.16        | 10.60        | 8.65         | 6.61         |
| rk3568 - u8              |              |              |              |              |
| rk3588 - u8(single core) | 53.73        | 33.24        | 22.31        | 14.74        |
| rk3588 - u8(triple core) |              |              |              |              |
