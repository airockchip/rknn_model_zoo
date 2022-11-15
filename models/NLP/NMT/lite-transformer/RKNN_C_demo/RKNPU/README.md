# Lite Transformer Demo

### 模型准备

- 参考 [文档](../../RKNN_model_convert/README.md) 将 onnx 模型转为 RKNN 模型
- 或者从 [网盘(密码:rknn)](https://eyun.baidu.com/s/3humTUNq) 下载RKNN模型，路径为models/NLP/NMT/lite-transformer/deploy_models/{dataset_name}/model_cvt/{platform}



### 编译

```sh
修改 build.sh 文件中 RV1109_TOOL_CHAIN/RK1808_TOOL_CHAIN的编译器位置
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
