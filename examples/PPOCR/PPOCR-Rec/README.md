# PPOCR-Rec

## Download ONNX model

Download link: 

[ppocrv4_rec.onnx](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/PPOCR/ppocrv4_rec.onnx)

Download with shell command:

```
cd model
./download_model.sh
```



## (Optional)PADDLE to ONNX

Please refer [Paddle_2_ONNX.md](Paddle_2_ONNX.md) 



## Model Convert

*Usage:*

```
cd python
python convert.py <onnx_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)>
# such as: python convert.py ../model/ppocrv4_rec.onnx rk3566
# output model will be saved as ../model/ppocrv4_rec.rknn
```

*Description:*

- <onnx_model> should be the ONNX model path.
- <TARGET_PLATFORM>  could be specified as RK3562, RK3566, RK3568, RK3588 according to board SOC version.
- <dtype\> is *optional*, could be specified as `i8` or `fp`, `i8` means to do quantization, `fp` means no to do quantization, default is `fp`.
- <output_rknn_path> is **optional**, used to specify the saving path of the RKNN model, default save path is `../model/ppocrv4_rec.rknn`



## Script Usage

For ONNX:

```bash
pip install -r python/requirements.txt
python python/ppocr_rec.py \
    --image_dir model/word_1.png \
    --rec_model_dir model/ch_PP-OCRv4_rec_infer/ppocrv4_rec.onnx \
    --rec_char_dict_path model/ppocr_keys_v1.txt \
    --use_gpu false --use_onnx true --rec_image_shape "3, 48, 320"
```

For RKNN:

```bash
python python/ppocr_rec.py \
    --image_dir model/word_1.png \
    --rec_model_dir model/ch_PP-OCRv4_rec_infer/ppocrv4_rec.rknn \
    --rec_char_dict_path model/ppocr_keys_v1.txt \
    --use_gpu false --use_rknn true --platform rk3568 --rec_image_shape "3, 48, 320"
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
./build-android.sh -t <TARGET_PLATFORM> -a arm64-v8a -d PPOCR-Rec
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board

Connect the USB port to PC, then push all demo files to the board.

```sh
adb root
adb push install/<TARGET_PLATFORM>_android_arm64-v8a/rknn_PPOCR-Rec_demo /data/
```

### Running

```sh
adb shell
cd /data/rknn_PPOCR-Rec_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_rec_demo model/ppocrv4_rec.rknn model/word_1.png
```

## Aarch64 Linux Demo

### Compiling && Building

According to the target platform, modify the path of 'GCC_COMPILER' in 'build-linux.sh'.

```sh
export GCC_COMPILER=/opt/tools/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
```

Then, run the script:

```sh
./build-linux.sh  -t <TARGET_PLATFORM> -a aarch64 -d PPOCR-Rec
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board


Push `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Rec_demo` to the board,

- If use adb via the EVB board:

```
adb push install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Rec_demo /userdata/
```

- For other boards, use the scp or other different approaches to push all files under `install/<TARGET_PLATFORM>_linux_aarch64/rknn_PPOCR-Rec_demo` to `/userdata`.

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Running

```sh
adb shell
cd /data/rknn_PPOCR-Rec_demo

export LD_LIBRARY_PATH=./lib
./rknn_ppocr_rec_demo model/ppocrv4_rec.rknn model/word_1.png
```

Note: Try searching the location of librga.so and add it to LD_LIBRARY_PATH if the librga.so is not found in the lib folder.
Use the following command to add it to LD_LIBRARY_PATH.

```sh
export LD_LIBRARY_PATH=./lib:<LOCATION_LIBRGA>
```

## Expected Results
This example will print the recognition result of the test image, as follows:
```
regconize result: JOINT, score=0.709082
```

- Note: Different platforms, different versions of tools and drivers may have slightly different results.