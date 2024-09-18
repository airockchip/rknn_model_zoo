# Guidelines for exporting clip onnx models

## Export clip onnx model

1. After meeting the installation environment requirements in the original `huggingface/openai/clip-vit-base-patch32` repository for exporting the onnx model, execute the following command to export the onnx model:

```sh
optimum-cli export onnx --model weights_path/ --task image-to-text --opset 18 path_to_output/

# weights_path refers to the original model file
# image-to-text refers to the type of the model
# path_to_output refers to the path where the decoder model will be saved
After execution, the model.onnx will be generated in the path_to_output.
```

2. Export clip text and images model
```sh
cd python
python truncated_onnx.py --model path_to_output/

After execution, the clip_text.onnx  and clip_images will be generated in the path_to_output.
```

*Note*

To remove the attention_mask input from the clip_text model, you can use the [onnx-modify](https://github.com/ZhangGe6/onnx-modifier) tool.