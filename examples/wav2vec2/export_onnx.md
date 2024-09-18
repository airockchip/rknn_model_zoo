
# Guidelines for exporting wav2vec2 onnx models

## Table of contents
- [Export wav2vec2 onnx model](#export-wav2vec2-onnx-model)
- [Special Notes](#special-notes)


## Export wav2vec2 onnx model

1.Install environment

```sh
pip install torch==1.10.0+cpu torchvision==0.11.0+cpu torchaudio==0.10.0 -f https://download.pytorch.org/whl/torch_stable.html

pip install transformers==4.39.3
```

2.Export onnx model
```sh
cd python
python export_onnx.py --chunk_length <CHUNK_LENGTH>

# such as:  
python export_onnx.py --chunk_length 20
```

*Description:*
- <CHUNK_LENGTH>: Specify the input audio length (in seconds). Such as `10`, `20`, `30`.


## Special Notes
1.About Python Demo
- The value of `CHUNK_LENGTH` in `wav2vec2.py` should be modified according to the input audio length.

2.About CPP Demo
- The value of `CHUNK_LENGTH` in `process.h` should be modified according to the input audio length.