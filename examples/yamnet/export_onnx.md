
# Guidelines for exporting yamnet onnx models

## Table of contents
- [Export yamnet onnx model](#export-yamnet-onnx-model)
- [Special Notes](#special-notes)



## Export yamnet onnx model

1.Install environment

```sh
pip install onnx==1.16.0
pip install onnxruntime==1.17.3
pip install tensorflow==2.13.1
pip install tensorflow_hub==0.16.1
pip install tf2onnx==1.16.1
```

2.Export onnx model
```sh
cd python
python export_onnx.py --chunk_length <CHUNK_LENGTH>

# such as:  
python export_onnx.py --chunk_length 3
```

*Description:*
- <CHUNK_LENGTH>: Specify the input audio length (in seconds). Such as `3`, `6`.


## Special Notes
1.About Python Demo
- The value of `CHUNK_LENGTH` in `yamnet.py` should be modified according to the input audio length.

2.About CPP Demo
- The value of `CHUNK_LENGTH` in `process.h` should be modified according to the input audio length.