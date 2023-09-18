## 框架便捷推理库

实现各框架的推理函数



## 基本用法：

```python
from executor import Excuter

model = Excuter(framework='onnx', model_path='model.onnx')
result = model.inference(inputs)
```



## 目前已实现：

- [x] caffe
  - [x] 调用docker执行 caffe run. 需要 caffe:cpu 镜像，获取地址: TODO[33]
  - [x] 调用docker生成 caffemodel，需要 caffe:torch 镜像，获取地址: TODO[33]
- [x] darknet
- [x] mxnet
- [x] ONNX
- [x] PyTorch
- [x] keras
- [x] tensorflow
- [x] tflite
