import os

# os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/yolox.torchscript.pt --coco_map_test --model yolox')


# os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/yolov5s.torchscript --RK_anchors ./RK_anchors_yolov5.txt --coco_map_test')
os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/yolov5_rv1126_u8.rknn --anchors ./anchors_yolov5.txt --coco_map_test --target rv1126')

# os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --coco_map_test --model yolox --target rv1126')

# os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/yolov7-tiny.torchscript.pt --RK_anchors ./RK_anchors_yolov7.txt --coco_map_test')
# os.system('python yolo_map_test_rknn.py --model_path ../RKNN_model_convert/model.rknn --RK_anchors ./RK_anchors_yolov7.txt --coco_map_test --target rv1126 --device_id 1126')

