<!--- SPDX-License-Identifier: Apache-2.0 -->

# RKNN Model Zoo
​	Rockchip Neural Network (RKNN) is a set of model formats defined by Rockchip based on its own NPU hardware architecture in order to accelerate model inference. On Rockchip NPU, models defined in this format can achieve much higher performance.
​	RKNN Model Zoo is a collection of SOTA models covering different tasks and different frameworks contributed by community members. It covers the complete AI application development process based on Rockchip NPU such as model conversion, model evaluation and model deployment. Each model include scripts related to **model conversion, model evaluation, and model deployment**. If there are code adjustments to the original project of the model, corresponding scripts or patches will also be provided.

## Model Include

#### Vision
* [image_classification](#image_classification)
* [image_segmentation](#image_segmentation)
* [object_detection](#object_detection)
* [face_analysis](#face_analysis)
* [body_analysis](#body_analysis)

### Image Recognition <a name="image_classification"/>
Models used for distinguish object in the picture. Refer [here](https://en.wikipedia.org/wiki/Outline_of_object_recognition)  
Model update later.  

### Image Segmentation <a name="image_segmentation"/>
Models used for locate objects and boundaries. Refer [here](https://en.wikipedia.org/wiki/Image_segmentation).

Model update later.  

### Object Detection <a name="image_segmentation"/>
Models used for detect object class and position in the picture. Refer [here](https://en.wikipedia.org/wiki/Object_detection)  

|Model |Discription |
|-|-|
|<b>[YOLOv5](models/vision/object_detection/yolov5-pytorch)</b>|YOLOv5 got a well balance of speed and accuracy on COCO benchmark|


<hr>

### Face Analysis<a name="face_analysis"/>
Models used for tasks related to human face, such as face landmark/recognition.

Model update later.  

### Body Analysis<a name="body_analysis"/>
Models used for tasks related to body, such as hand/body landmark. 
Model update later.  

