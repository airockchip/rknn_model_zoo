## Model Convert:

Here we using rk1808 and the yolov5s pretrained weight to make an example. Notice you need to export the yolov5s.torchscript model first.

In yolov5s_convert.yml config file:

- Param - RK_device_platform default value is RK1808. Change it to your device if need.
- Param - quantize default value is on. So prepare the COCO eval2017 dataset first. Refer to [TODO]. Set it to 'off' if you want a float RKNN model.
- Param - precompile default value is off. Change it to 'online' and connect to a npu device if you want to use precompile function.
- Param - RK_device_id default value is '1808'. Change it to your device id, getting via command `adb devices`. If you want to use simulator(only valid on linux system), set it to 'simulator'.
- For the model trained with others dataset, set the Param - model_file_path to your torchscript model, change the Param - input shape to 3,h,w. Like 3,736,1280.



Execute the following command to export RKNN model:

```python
python ../../../../../common/rknn_converter/rknn_convert.py --yml_path ./yolov5s_convert.yml --compute_convert_loss --eval_perf
```



## Model inference speed

Using rknn.eval API to evaluate speed.

- Model input shape: 1x3x640x640

- RKNN_toolkit1-1.7.1 (For rk3399pro, rk1808, rv1109, rv1126)

- RKNN_toolkit2-1.2.0 (For rk3566, rk3568, rk3588)

- Device info:

  - rk3399pro
  - rk1808:
    - Production: RK_EVB_RK1808_LP3D
    - Driver: 1.7.1
  - rv1109:
    - Production: RV1109 DDR3 EVB
    - Driver: 1.7.1
    - Other condition: Close ISP demo to avoid unwanted influence.
  - rv1126:
    - Production: RV1126 DDR3 EVB
    - Driver: 1.7.1
    - Other condition: Close ISP demo to avoid unwanted influence.
  - RK3566
    - Production: RK_EVB_RK3566_LP4XD200P132SD6_V11
    - Driver: 1.2.0
  - RK3568
  - RK3588
    - Production: RK_EVB2_RK3588_LP4XD
    - Driver: 1.2.0
  
  | platform（fps）          | yolov5s-relu | yolov5s-silu | yolov5m-relu | yolov5m-silu |
  | ------------------------ | ------------ | ------------ | ------------ | ------------ |
  | rk1808 - u8              | 35.24        | 26.41        | 16.27        | 12.60        |
  | rv1109 - u8              | 19.58        | 13.33        | 8.11         | 5.45         |
  | rv1126 - u8              | 27.54        | 19.29        | 11.69        | 7.86         |
  | rk3566 - u8              | 15.16        | 10.60        | 8.65         | 6.61         |
  | rk3568 - u8              |              |              |              |              |
  | rk3588 - u8(single core) | 53.73        | 33.24        | 22.31        | 14.74        |
  | rk3588 - u8(triple core) |              |              |              |              |