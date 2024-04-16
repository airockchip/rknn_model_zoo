- [1. Common Issue](#1-common-issue)
  - [1.1 How to upgrade RKNPU’s related dependent libraries](#11-how-to-upgrade-rknpus-related-dependent-libraries)
  - [1.2 Result differs via platform](#12-result-differs-via-platform)
  - [1.3 The demo result not correct](#13-the-demo-result-not-correct)
  - [1.4 The demo ran correctly, but when I replaced it with my own model, it ran wrong.](#14-the-demo-ran-correctly-but-when-i-replaced-it-with-my-own-model-it-ran-wrong)
  - [1.5 Model inference performance does not meet reference performance](#15-model-inference-performance-does-not-meet-reference-performance)
  - [1.6 How to solve the accuracy problem after model quantization](#16-how-to-solve-the-accuracy-problem-after-model-quantization)
  - [1.7 Is there a board-side python demo](#17-is-there-a-board-side-python-demo)
  - [1.8 Why are there no demos for other models? Is it because they are not supported](#18-why-are-there-no-demos-for-other-models-is-it-because-they-are-not-supported)
  - [1.9 Is there a LLM model demo?](#19-is-there-a-llm-model-demo)
  - [1.10 Why RV1103 and RV1106 can run fewer demos](#110-why-rv1103-and-rv1106-can-run-fewer-demos)
- [2. RGA](#2-rga)
- [3. YOLO](#3-yolo)
  - [3.1 Class confidence exceeds 1](#31-class-confidence-exceeds-1)
  - [3.2 So many boxes in the inference results and fill the entire picture](#32-so-many-boxes-in-the-inference-results-and-fill-the-entire-picture)
  - [3.3 The box's position and confidence are correct, but size not match well(yolov5, yolov7)](#33-the-boxs-position-and-confidence-are-correct-but-size-not-match-wellyolov5-yolov7)
  - [3.4 MAP accuracy is lower than official results](#34-map-accuracy-is-lower-than-official-results)
  - [3.5 Can NPU run YOLO without modifying the model structure?](#35-can-npu-run-yolo-without-modifying-the-model-structure)



## 1. Common Issue

### 1.1 How to upgrade RKNPU’s related dependent libraries

|              | RKNPU1                                                       | RKNPU2                                                       |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| Platform     | RV1109<br />RV1126<br />RK1808<br />RK3399pro                | RV1103<br />RV1106<br />RK3562<br />RK3566<br />RK3568<br />RK3588<br />RK3576 |
| Driver       | Upgrade by replace .ko file                                  | Upgrade by burning new firmware                              |
| Runtime      | Refer to the [documentation](https://github.com/airockchip/rknpu/blob/master/README.md) and replace **librknn_runtime.so** and its related dependency files to upgrade.<br /><br />(PC-to-board debugging function requires updating the **rknn_server** file described in the document) | Refer to [Document](https://github.com/rockchip-linux/rknn-toolkit2/blob/master/doc/rknn_server_proxy.md), replace the **librknnrt.so** file to upgrade<br />(RV1103/RV1106 use the cropped version runtime, the corresponding file name is **librknnmrt.so**)<br /><br />(PC-to-board debugging function requires updating the **rknn_server** file described in the document) |
| RKNN-Toolkit | Refer to the [documentation](https://github.com/airockchip/rknn-toolkit/blob/master/README.md) to install the new python whl file for upgrade | Refer to the [documentation](https://github.com/rockchip-linux/rknn-toolkit2/blob/master/doc/02_Rockchip_RKNPU_User_Guide_RKNN_SDK_V1.6.0_EN.pdf) **Chapter 2.1** to install the new python whl file for upgrade |

- Please note that due to differences in specifications of development boards, firmware is usually incompatible with each other. Please contact the source of purchase to obtain new firmware and burning methods.



### 1.2 Result differs via platform

Affected by the NPU generation, it is normal for the inference results to be slightly different. If the difference is significant, please submit an issue for feedback.



### 1.3 The demo result not correct

Please check whether the versions of the **driver**, runtime.so, and **RKNN-Toolkit** meet the version requirements listed in the [Document](README.md).



### 1.4 The demo ran correctly, but when I replaced it with my own model, it ran wrong.

Please check whether the requirements of the demo document are followed when exporting the model. For example, follow the [document](./examples/yolov5/README.md) and using https://github.com/airockchip/yolov5 to export onnx model.



### 1.5 Model inference performance does not meet reference performance

The following factors may cause differences in inference performance:

- The inference performance of **python api** may be weaker, please test the inference performance based on **C api**.

- The inference performance data of rknn model zoo does **not include pre-processing and post-processing**. It only counts the time-consuming of **rknn.run**, which is different from the time-consuming of the complete demo. The time-consuming of these other operations is related to usage scenarios and system resource occupancy.
- Whether the board has been **fixed frequency and reached the maximum frequency** set by [scaling_frequency.sh](./scaling_frequency.sh). Some firmware may limit the maximum frequency of the CPU/NPU/DDR, resulting in reduced inference performance.
- Whether there are other applications occupying **CPU/NPU and bandwidth resources**, which will cause the inference performance to be lower.
- For chips with both big and small core CPUs (currently RK3588, RK3576), please refer to [document](https://github.com/rockchip-linux/rknn-toolkit2/blob/master/doc/02_Rockchip_RKNPU_User_Guide_RKNN_SDK_V1.6.0_EN.pdf) Chapter 5.3.3, **bind the big CPU core for testing**.



### 1.6 How to solve the accuracy problem after model quantization

Please refer to the [userguide document](https://github.com/airockchip/rknn-toolkit2/blob/master/doc/02_Rockchip_RKNPU_User_Guide_RKNN_SDK_V1.6.0_CN.pdf) to confirm whether the quantization function is used correctly.

If the **model structural** characteristics and **weight distribution** cause int8 quantization to lose accuracy, please consider using **hybrid quantization** or **QAT quantization**.



### 1.7 Is there a board-side python demo

Install **RKNN-Toolkit-lite** on the board end and use the python inference script corresponding to the demo to implement python inference on the board. For RKNPU1 devices, refer to [RKNN-Toolkit-lite](https://github.com/airockchip/rknn-toolkit/tree/master/rknn-toolkit-lite). For RKNPU2 devices, refer to [RKNN-Toolkit-lite2](https://github.com/airockchip/rknn-toolkit2/tree/master/rknn_toolkit_lite2).

(Some examples currently lack python demo. In addition, it is recommended that users who care about performance use the C interface for deployment)



### 1.8 Why are there no demos for other models? Is it because they are not supported

Actually most of the models are supported. Limited by the development period, consideration for the needs of most developers, we have selected models with higher practicality as demo examples. If you have better model recommendations, please feel free to submit an issue or contact us.



### 1.9 Is there a LLM model demo?

Not provided now. Support for the transformer model is still being gradually optimized, and we also hope to provide large model demos for developers to refer to and use as soon as possible.



### 1.10 Why RV1103 and RV1106 can run fewer demos

Due to the memory size limit of **RV1103** and **RV1106**, the **memory** usage of many larger models exceeds the board limit, so corresponding demos are not provided now.



## 2. RGA

For RGA related issues, please refer to [RGA documentation](https://github.com/airockchip/librga/blob/main/docs/Rockchip_FAQ_RGA_EN.md).




## 3. YOLO

### 3.1 Class confidence exceeds 1

The **post-processing code** of the YOLO should matches with the model structure, otherwise an exception will occur. The post-process used by the current YOLO demo requires that the **category confidence output** of the model is **generated by sigmoid op**. The sigmoid op adjusts the confidence of (-∞, ∞) to the confidence of (0, 1). The lack of this sigmoid op will cause the category confidence greater than 1, as shown below:

![yolov5_without_sigmoid_out](asset/yolov5_without_sigmoid_out.png)

When encountering such problems, please refer to the instructions of the demo document to export the model.



### 3.2 So many boxes in the inference results and fill the entire picture

![yolo_too_much_box](asset/yolo_too_much_box.png)

As shown above, there are two possibilities:

- Same as the 3.1 section. Missing sigmoid op at the tail of the model may cause this problem.
- **Box threshold** value set too small and the **NMS threshold** value set too large in demo.



### 3.3 The box's position and confidence are correct, but size not match well(yolov5, yolov7)

This problem is usually caused by **anchor mismatch**. When exporting the model, please pay attention to whether the printed anchor information is inconsistent with the default anchor configuration in the demo.



### 3.4 MAP accuracy is lower than official results

There are two main reasons:

- The official map test uses **dynamic shape model**, while rknn model zoo uses a **fixed shape model** for simplicity and ease of use, and the map test results will be lower than the dynamic shape model.
- The RKNN model will also suffer some accuracy loss after **quantization** is turned on.

- If the user tries to use the C interface to test the map results on the board, please note that the way to read the image will affect the test results. For example, the map results are different when testing based on **cv2** and **stbi**.



### 3.5 Can NPU run YOLO without modifying the model structure?

**Yes, but not recommended.**

If the model structure changes, the post-processing code corresponding to python demo and Cdemo needs to be adjusted.

In addition, the adjustment method of the model structure was based on accuracy and performance considerations. Maintaining the original model structure may lead to **poor quantification accuracy** and **worse inference performance**.

