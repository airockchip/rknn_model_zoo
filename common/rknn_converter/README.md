## 模型转换工具

### 1.主要功能

- 以配置yml文件的形式，方便用户将其他模型转为 RKNN 模型
- 兼容 RKNN-Toolkit1/RKNN-Toolkit2
- 包含 模型转换、模型预编译、RKNN模型与原模型结果比对、记录 rknn api 耗时、生成测试结果记录。



### 2.推荐使用方法

如果不熟悉 vscode/ pycharm 等代码编辑器的调试功能，可以参考以下方式在命令行中使用 rknn_converter 工具

1. 执行 rknn_model_zoo/common/rknn_converter/regist_rkcvt.sh, 获取注册代码, 会显示类似以下字段

   ```
   Please execute the following command to set environment variables:
   export RKCVT=/home/xz/Documents/rknn_model_zoo/common/rknn_converter/rknn_convert.py
   
   Optional:
   export RKCVT_INIT=/home/xz/Documents/rknn_model_zoo/common/rknn_converter/config_init.py
   Example config: 
   /home/xz/Documents/rknn_model_zoo/common/rknn_converter/example_model/shufflenet_config.yml
   ```

2. 执行 `export RKCVT={path}/rknn_convert.py` 语句将 rknn_converter.py 定义为环境变量以便后续使用

3. 利用上述的 测试 RKCVT 功能各项功能，并在当前目录下生成测试报告 report.yml

   ```
   python $RKCVT --yml_path {Example config path} --eval_all --target_platform {platform name} --report --generate_random_input
   ```

4. 如需生成基础配置文件，参考以下方式生成

```
export RKCVT_INIT={path}/config_init.py
(python $RKCVT_INIT	--help 可看到输入选项，这里以onnx模型、rv1126平台，u8量化为例)
python $RKCVT_INIT onnx rv1126 u8
```



### 3.rknn_converter执行参数说明

| 参数名                  | 作用                                                         |
| ----------------------- | ------------------------------------------------------------ |
| --yml_path(必填)        | 指定yml配置文件路径                                          |
| --eval_perf             | 评估性能(基于rknn.eval_perf)                                 |
| --eval_memory           | 评估内存(基于rknn.eval_memory)                               |
| --python_api_test       | 测试 rknn-toolkit.run 与 framework.run 结果的余弦值          |
| --capi_test             | 测试 capi 与 framework.run结果余弦值，记录 input_set, run, output_get 耗时 |
| --capi_zero_copy_test   | 测试 capi 与 framework.run结果余弦值，记录耗时（目前可能有bug） |
| --eval_all              | 测试所有测试项目                                             |
|                         |                                                              |
| --target_platform       | 指定平台，不设置时会以 yml 文件中的指定平台为主              |
| --generate_random_input | 生成随机输入，方便在缺失输入数据的时候进行测试               |
| --report                | 生成报告                                                     |



生成报告示例:

```yaml
Board_info:
  chipname: RV1126
  device_id: 1126
  system: linux
  librknn_runtime_version: 1.7.3 (2e55827 build: 2022-08-25 10:45:32 base: 1131)
  CPU_freq:
    try_set: 1512000
    query: 1512000

  DDR_freq:
    try_set: not support setting
    query: None

  NPU_freq:
    try_set: 934000000
    query: 934000000

Model_info:
  framework: pytorch
  src_model: ./yolov7-tiny_no_sigmoid.pt
  src_model_md5: 211c19243ef534cc770096f3e21b4d47
  rknn_model: ./model_cvt/RV1109_1126/yolov7-tiny_no_sigmoid_RV1109_1126_u8_precompile.rknn
  rknn_model_md5: d5c6ca781434e7f5c485505d807c5d24
  dtype: asymmetric_affine-u8
  input_shape:
    in_0: [3, 640, 640]

  output_shape:
    output_0: [1, 255, 80, 80]
    output_1: [1, 255, 40, 40]
    output_2: [1, 255, 20, 20]

  Memory_info(MiB):
    weight: 12.54
    internal: 11.32
    total: 23.86
    model_file_size: 14.71

  Python_api:
    time_cost(ms):
      init: 331.55
      run(include adb/ntp data transmission): 181.22
      eval_performance(only model inference): 39.42

    accuracy(cos simularity):
      output_0: 0.9982245
      output_1: 0.998036
      output_2: 0.9985863

  RKNN_api(normal):
    time_cost(ms):
      model_init: 46.974998
      input_set: 4.0603
      run: 38.896599
      output_get: 38.969299
      total(except init): 81.926198

    accuracy(cos simularity):
      output_0: 0.9982245
      output_1: 0.99803627
      output_2: 0.9985863

  RKNN_api(zero_copy):
    time_cost(ms):
      model_init: 47.450001
      input_io_init: 0.587
      output_io_init: 0.328
      run: 39.200001
      total(except init): 39.200001

    accuracy(cos simularity):
      output_0: 0.9982245
      output_1: 0.998036
      output_2: 0.9985863
```





## 附录 - yml可填配置参数说明

| 参数名                                | 填写内容                                                     |
| ------------------------------------- | ------------------------------------------------------------ |
| model_name(选填)                      | 导出 rknn 模型的名称                                         |
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
| - out_0(tensorflow模型必填)           | 对于多输出的，请依次命名为 out_0,out_1,...,out_n             |
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

