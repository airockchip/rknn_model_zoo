# PPOCR-Rec Paddle To ONNX

## Download PP-OCRv4_rec

```bash
cd model
wget -c https://paddleocr.bj.bcebos.com/PP-OCRv4/chinese/ch_PP-OCRv4_rec_infer.tar
tar -xvf ch_PP-OCRv4_rec_infer.tar
rm ch_PP-OCRv4_rec_infer.tar
cd ..
```



## Convert paddle to ONNX

Install [paddle2onnx](https://github.com/PaddlePaddle/PaddleOCR/blob/release/2.7/deploy/paddle2onnx/readme.md)

```bash
paddle2onnx --model_dir ./model/ch_PP-OCRv4_rec_infer \
--model_filename inference.pdmodel \
--params_filename inference.pdiparams \
--save_file ./model/ch_PP-OCRv4_rec_infer/model.onnx \
--opset_version 12 \
--enable_dev_version True

# Seting fix input shape
python -m paddle2onnx.optimize --input_model model/ch_PP-OCRv4_rec_infer/model.onnx \
                               --output_model model/ch_PP-OCRv4_rec_infer/ppocrv4_rec.onnx \
                               --input_shape_dict "{'x':[1,3,48,320]}"
```
