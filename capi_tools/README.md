# Capi Tools 说明:

### 1. 定频工具

##### 	文件: scaling_frequency.sh

##### 	用途: 将 NPU/CPU/DDR 定频并校验定频结果，方便用户测试芯片性能

##### 	支持平台: 

​		一代NPU: RV1109, RV1126, RK1808

​			二代NPU: RK3566, RK3568, RK3588

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

