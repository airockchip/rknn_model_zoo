# Capi Tools 说明:

## 1. 定频工具

##### 	文件: scaling_frequency.sh

##### 	用途: 将 NPU/CPU/DDR 定频并校验定频结果，方便用户测试芯片性能

##### 	支持平台: 

​		一代NPU: RV1109, RV1126, RK1808

​		二代NPU: RK3566, RK3568, RK3588, RK3562, RV1103, RV1106

##### 	使用方法(以rk3566为例)

```
#For Linux
adb push scaling_frequency.sh /userdata/
adb shell
chmod 777 /userdata/scaling_frequency.sh
bash /userdata/scaling_frequency.sh -c rk3566

#For Android
adb root & adb remount
adb push scaling_frequency.sh /data/
chmod 777 /data/scaling_frequency.sh
/data/scaling_frequency.sh -c rk3566
```

**默认定频到最高频率, 如需使用其他频率:**

```
请修改 scaling_frequency.sh 中对应芯片平台的定频配置，这里以 rv1126 为例子，可以找到如下代码
if [ $chip_name == 'rv1126' ]; then
    seting_strategy=1
    CPU_freq=1512000
    NPU_freq=934000000
    DDR_freq=924000000
修改对应的数值配置，即可修改定频的频率。
```



## 2.CAPI 测试工具

CAPI测试工具帮助开发者快速验证板端的CAPI接口的可执行性、各接口的调用耗时。通常该接口被 RKNN CONVERTER 功能所使用，默认使用发布版本的 librknnrt.so 进行验证。如在 RKNPU2 对应平台需替换成其他版本的，请替换demo中对应的 rt.so 文件。

```
Toolkit2 runtime path:
./rknn_model_zoo/capi_tools/toolkit2/rknn_capi_test/install/{platform}/{system_type}/rknn_capi_test/lib/librknnrt.so

platform 代表芯片平台，如 rk3588, rv1106
system_type 代表板端系统类型，如安卓、Linux
```

```
测试工具使用方法:
rknn_capi_test ./xxx.rknn 
即可测试模型的推理耗时
```

