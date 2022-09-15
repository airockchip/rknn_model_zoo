## yml配置参数说明

| 参数名                                | 填写内容                                                     |
| ------------------------------------- | ------------------------------------------------------------ |
| model_name(必填)                      | 模型名称（这里实际不影响其他参数，只作为标识符使用）         |
| model_platform(必填)                  | 原模型的框架名（如caffe, darknet, keras, mxnet, onnx, pytorch, tensorflow, tflite） |
| model_file_path(必填)                 | 原模型路径（可填相对路径）                                   |
| RK_device_platform(必填)              | 目标npu设备平台，可填 [rk3399pro/rk1808/rv1109/rv1126/rk3566/rk3568/rk3588] |
| RK_device_id                          | npu设备id(可以通过abd devices获取)，仅连接单个npu设备的时候可不填，默认为None |
| dataset                               | 量化数据集，具体填写格式参考demo或user_guide手册。           |
| quantize                              | 是否量化，填 [True/False]                                    |
| pre_compile                           | 预编译模型，填写 [off\online] <br>（仅RKNN_toolkit1生效）    |
|                                       |                                                              |
| graph                                 |                                                              |
| - in_0(必填)                          | 对于多输入的，请依次命名为 in_0,in_1,...,in_n                |
| - name(tensorflow模型必填)            | 输入节点名                                                   |
| - shape(必填)                         | 输入的尺寸，nchw/nhwc的格式取决于原框架的形式，如pytorch模型的 3,224,224 |
| - mean_values                         | 输入的均值归一数，如 123.675,116.28,103.53。对于各通道归一化数字相等的，允许填写单值，如 0,0,0 => 0 |
| - std_values                          | 输入的方差归一数，如 58.395,58.295,58.391。对于各通道归一化数字相等的，允许填写单值，如 255,255,255 => 255 |
| - img_type                            | 根据原模型输入类型，填写 RGB 或者 BGR，如果是非图片的数据，请勿填写 |
| - out_0(tensorflow模型必填)           | 对于多输入的，请依次命名为 in_0,in_1,...,in_n                |
| - name(tensorflow模型必填)            | 输出节点名                                                   |
|                                       |                                                              |
| config                                | 对应 rknn.config 的参数配置                                  |
| - quantized_dtype                     | 量化类型<br>RKNN_toolkit1: 可填写 [asymmetric_affine-u8, dynamic_fixed_point-i8, dynamic_fixed_point-i16]<br>RKNN_toolkit2: 可填写 [asymmetric_quantized-8] |
| - quantized_algorithm                 | 量化方法：可选['normal', 'mmse']，默认为 normal              |
| - optimization_level                  | 优化等级，默认为3                                            |
| - mmse_epoch                          | mmse迭代次数，默认为3<br/>（仅RKNN_toolkit1生效）            |
| - do_sparse_network                   | 使用稀疏化优化量化模型，默认为True，如果量化模型掉精度，可考虑设为 False<br/>（仅RKNN_toolkit1生效） |
| - quantize_input_node                 | 单独量化输入节点<br/>（仅RKNN_toolkit1生效）                 |
| - merge_dequant_layer_and_output_node | 合并输出节点与反量化层<br/>（仅RKNN_toolkit1生效）           |
| - force_builtin_perm                  | 为输入添加transpose layer使 nhwc -> nchw<br/>（仅RKNN_toolkit1生效） |
