## Python demo

- 支持YOLO模型单图测试、coco数据集的 benchmark 测试
- 支持 pytorch/rknn/onnx 格式的模型
- 支持 Yolo[v5, v6, v7, v8], ppyoloe_plus, YoloX 模型
- 注意: 由于 **RKNN 模型不支持动态输入**，该 demo 会对输入图片进行 letter_box 处理。与原始仓库使用动态输入(例如yolov5仓库)的预测结果作为对比，同一张图片的预测结果可能略有差异。
- 请注意，该版本使用的 yolo 模型包含尾部的sigmoid op，其他版本可能不包含sigmoid op，请勿混用，混用会导致结果异常。



## 准备工作

1.[datasets/COCO](../../../../../datasets/COCO) 目录下根据提示，下载相关数据

2.准备好测试模型，可以是 RKNN/ torchscript/ onnx 格式的模型，可参考 [README](../README.md) 里面的说明获取



## 执行demo

```
Yolov5:
python yolo_map_test_rknn.py --model yolov5 --model_path ./yolov5.pt --anchors anchors_yolov5.txt

Yolov6:
python yolo_map_test_rknn.py --model yolov6 --model_path ./yolov6.pt

Yolov7:
python yolo_map_test_rknn.py --model yolov7 --model_path ./yolov7.pt --anchors anchors_yolov7.txt

Yolov8:
python yolo_map_test_rknn.py --model yolov6 --model_path ./yolov8.pt

YOLOX:
python yolo_map_test_rknn.py --model yolox --model_path ./yolox.pt

ppyoloe_plus:
python yolo_map_test_rknn.py --model ppyoloe_plus --model_path ./yolox.pt
```

- 使用 rknn 模型作为测试模型时，需加上参数 `--target {platform} --device_id {device_id}`，以 rv1126(假设 device id 为 123456)，则参数为  `--target rv1126 --device_id 123456`
- 默认使用 coco eval 作为测试数据集，如使用其他文件夹下图片作为测试，加上参数 `--img_folder {folder_path} ` ，如 `--img_folder ../test_data`
- 想获取 COCO benchmark 测试结果时，加上参数  `--coco_map_test`
- 想看到图片画框效果，加上参数 `--img_show`

