## 框架便捷推理库

实现各框架的推理函数



## 基本用法：

```python
from excuter import Excuter

model = Excuter(framework='onnx', model_path='model.onnx')
result = model.inference(inputs)
```



## 目前已实现：

- [x] caffe
- [x] darknet
- [x] mxnet
- [x] ONNX
- [x] PyTorch
- [x] keras
- [x] tensorflow
- [x] tflite
