# 模型转换

将 lite-transformer-encoder-16.onnx, lite-transformer-decoder-16.onnx 放置于 model_file 目录下，执行

```
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./model_config_encoder.yml --python_api_test --capi_test
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./model_config_decoder.yml --python_api_test --capi_test
```



#### onnx 模型有以下获取方式：

- 参考[仓库](https://github.com/airockchip/lite-transformer/README_RK.md)训练得到
- 使用[网盘(密码rknn)](https://eyun.baidu.com/s/3humTUNq)提供的预训练模型，预训练权重路径为 models/NLP/NMT/lite-transformer/checkpoints/{dataset_name}

