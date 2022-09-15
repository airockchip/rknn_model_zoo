# Yolov5 - pytorch

- official respository: https://github.com/ultralytics/yolov5.git
- Support commit id: c9a46a6, **Now support v5,v6 model version**



## Clone repository and apply patch

​	In the folder containing this README_EN.md, excute the following command.

```sh
git clone https://github.com/ultralytics/yolov5.git
cd yolov5
git checkout c9a46a60e09ab94009754ca71bde23e91aab33fe
git apply ../patchs/rknpu_optimize.patch

#For rk3399pro,rk1808,rv1109,rv1126 devices, we recommand to apply the following patch. This will replace silu layer with relu layer to got better inference speed. NOTICE: Weight is not compatible between these layers.
#For rk3566,rk3568,rk3588 devices, the following patch is not necessary.
git apply ../patchs/silu_to_relu.patch
```



---

### Training：

​	Refer to the yolov5 official documents.

​	COCO dataset address:  https://cocodataset.org/#download

NOTICE:

- Model with Silu activation layer isn't compatible with ReLu. Don't mix the weight. The official weights use Silu.

- Except for the replacement of Silu with ReLU, other optimization operations only change the structure of some operators, and the optimization operation will not affect the prediction results of the model. Therefore, for how to train the model, you can refer to the official documentation, which will not be repeated here.

- For different datasets, the yolov5 repository may automatically find the best anchors. Please pay attention to the anchors information when exporting the model. If it is inconsistent with the anchors information of the subsequent demo, you need to modify the anchors information in the demo.

- Generally , the stride information of the Detect layer is fixed to [8, 16, 32]. If the definition is modified, the corresponding information needs to be modified in the demo.

  

---

### Export PT(torchscript)  model

- Execute the following command in the yolov5 directory to export the model optimized for RKNN (the model without post-processing structures). The anchors will be printed and saved as a txt file.

```python
python export.py --rknpu {device_platform}
```

- Change {device platform} value according to your npu device. Select from [rk1808/rv1109/rv1126/rk3399pro/rk3566/rk3568/rk3588]
- For different platform, optimize has difference



---

### Convert to RKNN model

Please refer to [./RKNN_model_convert/README_EN.md](./RKNN_model_convert/README_EN.md)



---

### Python_demo

Please refer to [./RKNN_python_demo/README_EN.md](./RKNN_python_demo/README_EN.md) 




---

### C_demo

For RKNN_toolkit1,  please refer to [./RKNN_C_demo/RKNN_toolkit_1/rknn_yolov5_demo/README_EN.md](./RKNN_C_demo/RKNN_toolkit_1/rknn_yolov5_demo/README_EN.md) 

For RKNN_toolkit2, please refer to TODO.




---

### Patch optimize detail:

- **Large-size Maxpool optimization**: The large-size Maxpool is not friendly to NPU, which will lead to increased time-consuming. From the formula, the 5x5 Maxpool is equivalent to two 3x3 Maxpools in series. So we decompose 5x5/9x9/13x13 Maxpool into multi 3x3 Maxpool layers, making the model more suitable for deployment on RKNN devices without compromising accuracy.
- **Focus layer uses convolution optimization**: The slice operator is currently unfriendly for some RKNN devices. We replace it with convolution with a special weight distribution to optimize the model deployment speed while ensuring that the calculation results remain unchanged.
- **Removing post-processing**: The post-processing of the model is not friendly to NPU devices, and some post-processing ops will have accuracy problems after quantization. So we remove the post-processing structure of the yolov5 model and remake it to python/C++ code.