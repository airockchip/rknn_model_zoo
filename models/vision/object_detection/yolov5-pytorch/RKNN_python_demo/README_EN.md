## Python demo

#### Feature

- support single image/ coco benchmark test.
- support pytorch/rknn model



## Prepare

- Refer to [doc](../README_EN.md) and export yolov5s.rknn to '../rknn_convert' path. 
- Refer to [doc](../../../../../datasets/README_EN.md) prepare datasets.



## Compare result with yolov5.detect.py

- yolov5/detect

  ```
  # In the yolov5 folder
  python detect.py --source ../test_data/
  # The result will save in yolov5/runs/detect
  ```

- RKNN python demo

  ```
  # In this file folder path
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --img_show --img_folder ../test_data
  # The result will display via windows.
  ```

  Compare with the results, we can see that inference result got a litter bit different for the some of the pictures. This is because yolov5/detect.py using dynamic shape input, while RKNN model needs input shape to be fixed. 
  
  

## Run demo

Here we take coco val2017 dataset and rk1808 as example.

- Single img:

  ```
  RKNN:
  python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --img_show --target rk1808
  
  Pytorch:
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --img_show
  ```

  

- coco benchmark:

  ```
  RKNN:
  python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --target rk1808 --coco_map_test
  
  Pytorch:
  python yolo_map_test_rknn.py --model_path ../yolov5/yolov5s.torchscript --coco_map_test
  ```



## COCO_benchmark test result

TODO
