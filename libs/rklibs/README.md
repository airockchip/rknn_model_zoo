## rklibs

在 rknn_model_zoo/libs/rklibs 目录下，根据实际需要，下载以下依赖库

```
git submodule init

# RK1808/RV1126/RV1109 NPU 依赖库
git submodule update rknpu

# RK3399pro NPU 依赖库
git submodule update RK3399Pro_npu

# RK3566/RK3568/RK3588/RV1106/RV1103 NPU 依赖库
git submodule update rknpu2

# RGA调用依赖库，不区分硬件平台
git submodule update librga
```

