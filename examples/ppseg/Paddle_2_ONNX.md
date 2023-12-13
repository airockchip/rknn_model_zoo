## Paddle to ONNX

1. Install Repository [PaddleSeg](https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8)

2. Refer [Deployment](https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.8/configs/pp_liteseg#deployment) , exporting ONNX model with command as follow:

   ```
   cd PaddleSeg
   python deploy/python/infer_onnx_trt.py \
       --config <path to config> \
       --model_path <path to model> \
       --width <img_width> \
       --height <img_height> 
   ```

   The ONNX model will be generated in ./output 
