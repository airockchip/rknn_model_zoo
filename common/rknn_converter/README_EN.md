## 模型转换工具

### Feature

- Using yml file as config to export RKNN model
- Include [model convert / online precomplie / convert accuracy test] function



### Yml parameters

| Param                                 | Type            | Value                                                        |
| ------------------------------------- | --------------- | ------------------------------------------------------------ |
| model_name(required)                  | str             | model name here. (Useless now)                               |
| model_platform(required)              | str             | Model training framework. (support caffe, darknet, keras, mxnet, onnx, pytorch, tensorflow, tflite） |
| model_file_path(required)             | str             | model path (support relative path)                           |
| RK_device_platform(required)          | str             | npu device, value selects from [rk3399pro/rk1808/rv1109/rv1126/rk3566/rk3568/rk3588] |
| RK_device_id                          | str             | NPU device id(get via adb devices). Only required if multi NPU device is connected. |
| dataset                               | str             | The path of Dataset for quantization. The file should be txt format. For more detail please refer to demo or user_guide |
| quantize                              | bool            | Quantize or not, value selects from [True/False]             |
| pre_compile                           | str             | Precompile for RKNN model, value select from [off\online] <br>（Only valid for RKNN_toolkit1） |
| graph                                 |                 |                                                              |
| - in_0(required)                      |                 | for multi input, named as in_0,in_1,...,in_n                 |
| - name(tensorflow model required)     | str             | input node name                                              |
| - shape(required)                     | str             | Input shape, for tensorflow, write in hwc format, like 224,224,3. For the others, write in chw format, like 3,224,224 |
| - mean_values                         | [str/float/int] | (Normalization) mean value for input. For 3 channel input, set like 123.675,116.28,103.53. If all channel share the same value, write in single value is allowed, like 0,0,0 => 0. |
| - std_values                          | [str/float/int] | (Normalization) std value for input. For 3 channel input, set like 58.395,58.295,58.391. If all channel share the same value, write in single value is allowed, like 255,255,255 => 255. |
| - img_type                            | str             | According to the input, select from [RGB/BGR] input. If not image input, DO NOT SET this parameters. |
| - out_0(tensorflow model required)    |                 | for multi output, named as out_0,out_1,...,out_n             |
| - name(tensorflow model required)     | str             | output node name                                             |
|                                       |                 |                                                              |
| config                                |                 | Parameters for rknn.config.                                  |
| - quantized_dtype                     | str             | quantize type<br>RKNN_toolkit1: select from [asymmetric_affine-u8, dynamic_fixed_point-i8, dynamic_fixed_point-i16]<br>RKNN_toolkit2: select from [asymmetric_quantized-8]<br>default as asymmetric_affine-u8/asymmetric_quantized-8 |
| - quantized_algorithm                 | str             | select from ['normal', 'mmse'], default as normal.           |
| - optimization_level                  | int             | model optimize level, select from [0,1,2,3], default as 3    |
| - mmse_epoch                          | int             | mmse iteration epoch number<br/>（Valid for RKNN_toolkit1）  |
| - do_sparse_network                   | bool            | Using sparse optimize for quantizing model. Default as True. If quantized model got accuracy drop, try to set False.<br/>（Valid for RKNN_toolkit1） |
| - quantize_input_node                 | bool            | Quantize input node, mostly used in loading QAT pytorch model<br/>（Valid for RKNN_toolkit1） |
| - merge_dequant_layer_and_output_node | bool            | Merge dequantize layer and output node, mostly used in loading QAT pytorch model<br/>（Valid for RKNN_toolkit1） |
| - force_builtin_perm                  | bool            | Add transpose layer to the head of model. nhwc -> nchw<br/>（Valid for RKNN_toolkit1） |



### 3. Script argument

`python rknn_convert.py` command has follow arguments.

| ARG                    | value                                                        |
| ---------------------- | ------------------------------------------------------------ |
| --yml_path(require)    | Yml config file path                                         |
| --output_path          | Output rknn path. If file already exists, script will query to rewrite or not. Default as ./model.rknn |
| --compute_convert_loss | Compare RKNN model inference result with origin model.       |
| --eval_perf            | Evaluate RKNN model inference speed                          |



### 4. Execute example.

```python
python {rknn_model_zoo/common/rknn_converted/rknn_convert.py} \
--yml_path ./model_config.yml \
--output_path ./model.rknn \
--compute_convert_loss \
--eval_perf
```

