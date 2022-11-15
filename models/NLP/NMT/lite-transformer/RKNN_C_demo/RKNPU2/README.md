# Lite Transformer Demo

## 获取rknn模型

- 参考 [文档](../../RKNN_model_convert/README.md) 将 onnx 模型转为 RKNN 模型
- 或者从 [网盘(密码:rknn)](https://eyun.baidu.com/s/3humTUNq) 下载RKNN模型，路径为models/NLP/NMT/lite-transformer/deploy_models/{dataset_name}/model_cvt/{platform}



## Android Demo

### 编译

根据指定平台修改 `build-android_<TARGET_PLATFORM>.sh`中的Android NDK的路径 `ANDROID_NDK_PATH`，<TARGET_PLATFORM>可以是RK356X或RK3588 例如修改成：

```sh
ANDROID_NDK_PATH=~/opt/tool_chain/android-ndk-r17
```

然后执行：

```sh
./build-android_<TARGET_PLATFORM>.sh
```



### 推送执行文件到板子

连接板子的usb口到PC,将整个demo目录到 `/data`:

```sh
adb root
adb remount
adb push install/rknn_lite_transformer_demo_Android /data/
```

### 运行

```sh
adb shell
cd /data/rknn_lite_transformer_demo_Android/

export LD_LIBRARY_PATH=./lib
./rknn_lite_transformer_demo model/${PLATFORM}/lite-transformer-encoder-16.rknn model/${PLATFORM}/lite-transformer-decoder-16.rknn
```



## Aarch64 Linux Demo

### 编译

根据指定平台修改 `build-linux_<TARGET_PLATFORM>.sh`中的交叉编译器所在目录的路径 `TOOL_CHAIN`，例如修改成

```sh
export TOOL_CHAIN=~/opt/tool_chain/gcc-9.3.0-x86_64_aarch64-linux-gnu/host
```

然后执行：

```sh
./build-linux_<TARGET_PLATFORM>.sh
```

### 推送执行文件到板子


将 install/rknn_lite_transformer_demo_Linux 拷贝到板子的/userdata/目录.

- 如果使用rockchip的EVB板子，可以使用adb将文件推到板子上：

```
adb push install/rknn_lite_transformer_demo_Linux /userdata/
```

- 如果使用其他板子，可以使用scp等方式将install/rknn_lite_transformer_demo_Linux拷贝到板子的/userdata/目录

### 运行

```sh
adb shell
cd /userdata/rknn_lite_transformer_demo_Linux/

export LD_LIBRARY_PATH=./lib
./rknn_lite_transformer_demo model/${PLATFORM}/lite-transformer-encoder-16.rknn model/${PLATFORM}/lite-transformer-decoder-16.rknn
```
