# PPOCR-System


## Prepare model

Refer [PPOCR-Det](../PPOCR-Det) and [PPOCR-Rec](../PPOCR-Rec) to get ONNX and RKNN models.

## Script Usage

Install libs:

```bash
pip install -r python/requirements.txt
```

For ONNX:

```bash
python python/ppocr_system.py \
    --image_dir model/test1.jpg \
    --det_model_dir ../PPOCR-Det/model/ppocrv4_det.onnx \
    --rec_model_dir ../PPOCR-Rec/model/ppocrv4_rec.onnx \
    --rec_char_dict_path ../PPOCR-Rec/model/ppocr_keys_v1.txt \
    --vis_font_path model/simfang.ttf \
    --use_gpu false --use_onnx true --rec_image_shape "3, 48, 320"
```

For RKNN:
```bash
python python/ppocr_system.py \
    --image_dir model/test1.jpg \
    --det_model_dir ../PPOCR-Det/model/ppocrv4_det.rknn \
    --rec_model_dir ../PPOCR-Rec/model/ppocrv4_rec.rknn \
    --rec_char_dict_path ../PPOCR-Rec/model/ppocr_keys_v1.txt \
    --vis_font_path model/simfang.ttf \
    --use_gpu false --use_rknn true --platform rk3568 --det_image_shape 480 480 --rec_image_shape "3, 48, 320"
```

## Android Demo

### Compiling && Building

Modify the path of Android NDK in 'build-android.sh'.

For example,

```sh
export ANDROID_NDK_PATH=~/opt/toolchain/android-ndk-r18b
```

Then, run this script:

```sh
./build-android.sh -t <TARGET_PLATFORM> -a arm64-v8a -d PPOCR-System
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board

Connect the USB port to PC, then push all demo files to the board.

```sh
adb root
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_PPOCR-System_demo /data/
```

### Running

```sh
adb shell
cd /data/rknn_PPOCR-System_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_system_demo model/ppocrv4_det.rknn model/ppocrv4_rec.rknn model/test1.jpg
```

## Aarch64 Linux Demo

### Compiling && Building

According to the target platform, modify the path of 'GCC_COMPILER' in 'build-linux.sh'.

```sh
export GCC_COMPILER=/opt/tools/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
```

Then, run the script:

```sh
./build-linux.sh  -t <TARGET_PLATFORM> -a aarch64 -d PPOCR-System
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board


Push `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-System_demo` to the board,

- If use adb via the EVB board:

```
adb push install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-System_demo /userdata/
```

- For other boards, use the scp or other different approaches to push all files under `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-System_demo` to `/userdata`.

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Running

```sh
adb shell
cd /data/rknn_PPOCR-System_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_system_demo model/ppocrv4_det.rknn model/ppocrv4_rec.rknn model/test1.jpg
```

Note: Try searching the location of librga.so and add it to LD_LIBRARY_PATH if the librga.so is not found in the lib folder.
Use the following command to add it to LD_LIBRARY_PATH.

```sh
export LD_LIBRARY_PATH=./lib:<LOCATION_LIBRGA>
```

## Expected Results
This example will print the box and text recognition results of the test image, as follows:
```
[0] @ [(28, 37), (302, 39), (301, 70), (27, 69)]
regconize result: 纯臻营养护发素, score=0.711147
[1] @ [(26, 82), (171, 82), (171, 104), (26, 104)]
regconize result: 产品信息/参数, score=0.709542
[2] @ [(27, 111), (332, 113), (331, 134), (26, 133)]
regconize result: （45元/每公斤，100公斤起订）, score=0.697409
[3] @ [(28, 142), (281, 144), (280, 163), (27, 161)]
regconize result: 每瓶22元，1000瓶起订）, score=0.702602
[4] @ [(25, 179), (298, 177), (300, 194), (26, 195)]
regconize result: 【品牌】：代加工方式/OEMODM, score=0.705021
[5] @ [(26, 209), (234, 209), (234, 227), (26, 227)]
regconize result: 【品名】：纯臻营养护发素, score=0.710205
...
```

<img src="result.jpg">

- Note: Different platforms, different versions of tools and drivers may have slightly different results.