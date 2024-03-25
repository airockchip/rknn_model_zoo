# 交叉编译环境搭建指南

需要搭建好交叉编译环境，才可以在x86 Linux系统上编译本工程示例中的C/C++ Demo。


## Android平台

目标设备是Android系统时，使用根目录下的`build-android.sh`脚本编译具体模型的C/C++ Demo。  
使用该脚本编译C/C++ Demo前需要先通过环境变量`ANDROID_NDK_PATH`指定Android NDK的路径。

### 下载Android NDK

*（如果系统中已经装有Android NDK，请忽略此步骤）*

1. 通过此链接下载NDK（建议下载r19c版本）：https://dl.google.com/android/repository/android-ndk-r19c-linux-x86_64.zip
2. 解压缩下载好的Android NDK。记住该路径，后面编译C/C++ Demo时会用到该路径。**注：上述NDK解压后的目录名为`android-ndk-r19c`。**

### 编译C/C++ Demo

编译C/C++ Demo的命令如下:
```shell
export ANDROID_NDK_PATH=<android_ndk_path>

./build-android.sh -t <TARGET_PLATFORM> -a <ARCH> -d <model_name>
# for RK3588:
./build-android.sh -t rk3588 -a arm64-v8a -d mobilenet
# for RK3566:
./build-android.sh -t rk3566 -a arm64-v8a -d mobilenet
# for RK3568:
./build-android.sh -t rk3568 -a arm64-v8a -d mobilenet
```
*参数说明:*
- `<android_ndk_path>`: 指定Android NDK路径，例如：`~/opt/android-ndk-r19c`。
- `<TARGET_PLATFORM>`: 指定目标平台，例如`rk3566`, `rk3568`, `rk3588`。**注：`RK1808`, `RV1109`, `RV1126`, `RV1103`, `RV1106`不支持`Android`平台。**
- `<ARCH>`: 指定系统架构。可以在目标设备执行如下命令查询系统架构:
	```shell
	# Query architecture. For Android, ['arm64-v8a' or 'armeabi-v7a'] should shown in log.
	adb shell cat /proc/version
	```
- `model_name`: 模型名，即examples目录下各个模型所在的文件夹名。


## Linux平台

目标设备是`Linux`系统时，使用根目录下的`build-linux.sh`脚本编译具体模型的 C/C++ Demo。  
使用该脚本编译C/C++ Demo前需要先通过环境变量`GCC_COMPILER`指定交叉编译工具的路径。

### 下载交叉编译工具

*（如果系统中已经装有交叉编译工具，请忽略此步骤）*

1. 不同的系统架构，依赖不同的交叉编译工具。下面给出具体系统架构建议使用的交叉编译工具下载链接：
   - aarch64: https://releases.linaro.org/components/toolchain/binaries/6.3-2017.05/aarch64-linux-gnu/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
   - armhf: https://developer.arm.com/-/media/Files/downloads/gnu-a/8.3-2019.03/binrel/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf.tar.xz?revision=e09a1c45-0ed3-4a8e-b06b-db3978fd8d56&rev=e09a1c450ed34a8eb06bdb3978fd8d56&hash=9C4F2E8255CB4D87EABF5769A2E65733
   - armhf-uclibcgnueabihf(RV1103/RV1106): https://console.zbox.filez.com/l/H1fV9a (fetch code: rknn)
2. 解压缩下载好的交叉编译工具，记住具体的路径，后面在编译时会用到该路径。

### 编译C/C++ Demo

编译C/C++ Demo的命令参考如下：
```shell
# go to the rknn_model_zoo root directory
cd <rknn_model_zoo_root_path>

# if GCC_COMPILER not found while building, please set GCC_COMPILER path
export GCC_COMPILER=<GCC_COMPILER_PATH>

./build-linux.sh -t <TARGET_PLATFORM> -a <ARCH> -d <model_name>

# for RK3588
./build-linux.sh -t rk3588 -a aarch64 -d mobilenet
# for RK3566
./build-linux.sh -t rk3566 -a aarch64 -d mobilenet
# for RK3568
./build-linux.sh -t rk3568 -a aarch64 -d mobilenet
# for RK1808
./build-linux.sh -t rk1808 -a aarch64 -d mobilenet
# for RV1109
./build-linux.sh -t rv1109 -a armhf -d mobilenet
# for RV1126
./build-linux.sh -t rv1126 -a armhf -d mobilenet
# for RV1103
./build-linux.sh -t rv1103 -a armhf -d mobilenet
# for RV1106
./build-linux.sh -t rv1106 -a armhf -d mobilenet
```

*参数说明:*
- `<GCC_COMPILER_PATH>`: 指定交叉编译路径。不同的系统架构，所用的交叉编译工具并不相同。
    - `GCC_COMPILE_PATH` 示例:
        - aarch64: ~/tools/cross_compiler/arm/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
        - armhf: ~/tools/cross_compiler/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf
        - armhf-uclibcgnueabihf(RV1103/RV1106): ~/tools/cross_compiler/arm/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf
- `<TARGET_PLATFORM>`: 指定目标平台。例如：`rk3588`。**注：每个模型当前支持的目标平台可能有所不同，请参考具体模型目录下的`README.md`文档。**
- `<ARCH>`: 指定系统架构。可以在目标设备执行如下命令查询系统架构: 
  ```shell
  # Query architecture. For Linux, ['aarch64' or 'armhf'] should shown in log.
  adb shell cat /proc/version
  ```
- `model_name`: 模型名，即examples目录下各个模型所在的文件夹名。
