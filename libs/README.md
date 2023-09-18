# Libs 说明

libs 文件夹存放 C demo 编译时依赖的库，目前分为以下子文件夹



## ./common

存放无需区分使用平台的依赖库



## ./platform

存放需区分使用平台的依赖库



## ./rklibs

存放 Rockchip 维护的依赖库，编译 C demo 前，请根据使用的NPU平台，执行以下指令进行初始化

```
cd rklibs

# RK1808/RV1126/RV1109 NPU 依赖库
git clone https://github.com/rockchip-linux/rknpu

# RK3399pro NPU 依赖库
git clone https://github.com/airockchip/RK3399Pro_npu

# RK3566/RK3568/RK3588/RV1106/RV1103 NPU 依赖库
git clone https://github.com/rockchip-linux/rknpu2

# RGA调用依赖库，不区分硬件平台
git clone https://github.com/airockchip/librga
```

