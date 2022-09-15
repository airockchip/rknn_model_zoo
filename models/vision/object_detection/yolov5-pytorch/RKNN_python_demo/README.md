## Python demo

- yolov5单图测试、coco数据集的benchmark测试。
- 支持 pytorch/rknn模型。



## 准备工作

1.已经有yolov5.rknn模型放置在../rknn_convert路径下，导出/转换模型相关请看对应目录下的README.md文档

2.rknn_model_zoo/datasets/COCO目录下根据提示下载好相关数据



## 校验demo与 yolov5.detect 实现差异

- yolov5.detect

  ```
  在yolov5仓库下执行
  python detect.py --source ../test_data/
  检测结果保存于 yolov5/runs/detect里面
  ```

- RKNN python demo

  ```
  在本文档目录下执行
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --img_show --img_folder ../test_data
  检测结果由cv2窗口绘出
  ```

  比较以上两者的结果，可以看到对于同一个yolov5s模型，在个别图片上检测结果略有偏差(该偏差为中性，检测结果变好变差都有可能)。这是因为yolov5.detect.py的实现允许动态尺寸输入导致的，RKNN模型目前不允许动态尺寸输入，所以最终的检测结果存在部分差异。



## 执行demo

默认使用coco eval作为测试数据集，这里以RK1808为例

- 逐图推理并绘出结果：

  ```
  RKNN:
  python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --img_show --target rk1808
  
  Pytorch:
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --img_show
  ```

  

- coco benchmark测试：

  ```
  RKNN:
  python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --target rk1808 --coco_map_test
  
  Pytorch:
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --coco_map_test
  ```



## COCO_benchmark测试结果

TODO
