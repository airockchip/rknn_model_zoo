
# Guidelines for exporting whisper onnx models

## Table of contents

- [Export whisper onnx model of 20 seconds length](#export-whisper-onnx-model-of-20-seconds-length)
- [Export whisper onnx model of original 30 seconds length](#export-whisper-onnx-model-of-original-30-seconds-length)
- [Special Notes](#special-notes)



## Export whisper onnx model of 20 seconds length

1.Modify the source code

```sh
pip install openai-whisper==20231117

# 1. Modify CHUNK_LENGTH in the whisper/audio.py installation package path, for example ~/python3.8/site-packages/whisper/audio.py
CHUNK_LENGTH = 30 
-->> CHUNK_LENGTH = 20

# 2. Modify positional_embedding in the whisper/model.py installation package path, for example ~/python3.8/site-packages/whisper/model.py
assert x.shape[1:] == self.positional_embedding.shape, "incorrect audio shape" 
-->> # assert x.shape[1:] == self.positional_embedding.shape, "incorrect audio shape"

x = (x + self.positional_embedding).to(x.dtype) 
-->> x = (x + self.positional_embedding[-x.shape[1]:,:]).to(x.dtype)
```

2.Export onnx model
```sh
cd python
python export_onnx.py --model_type <MODEL_TYPE> --n_mels <N_MELS(optional)>

# such as:  
python export_onnx.py --model_type base --n_mels 80
```

*Description:*
- <MODEL_TYPE>: Specify the model type. Such as `tiny`, `base`, `medium`.
- <N_MELS(optional)>: Specify the number of mels. Such as `80`, `128`. Default is `80`.

***Note:***<br>
`small` and `large` models are not supported yet.


## Export whisper onnx model of original 30 seconds length 

```sh
pip install openai-whisper==20231117

cd python
python export_onnx.py --model_type <MODEL_TYPE> --n_mels <N_MELS(optional)>

# such as:  
python export_onnx.py --model_type base --n_mels 80
```

*Description:*
- <MODEL_TYPE>: Specify the model type. Such as `tiny`, `base`, `medium`.
- <N_MELS(optional)>: Specify the number of mels. Such as `80`, `128`. Default is `80`.

***Note:***<br>
`small` and `large` models are not supported yet.


## Special Notes
1.About Python Demo
- The value of `CHUNK_LENGTH` in `whisper.py` should be modified according to the input length of the model.

2.About CPP Demo
- The value of `CHUNK_LENGTH` in `process.h` should be modified according to the input length of the model. 
- The value of `ENCODER_OUTPUT_SIZE` in `process.h` should be modified according to the model type. For example, modify `ENCODER_OUTPUT_SIZE` to `384/512/1024` corresponding to `tiny/base/medium`.