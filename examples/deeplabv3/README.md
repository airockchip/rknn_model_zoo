# Deeplabv3

## Model Source
Download link: 

[deeplab-v3-plus-mobilenet-v2.pb](https://ftrg.zbox.filez.com/v2/delivery/data/ec1c6f44f8c24155875ac5bce7aa6b3c/examples/Deeplabv3/deeplab-v3-plus-mobilenet-v2.pb)

Download with shell command

```shell
cd model
./download_model.sh
```

## Python Script Usage
*Usage:*

```shell
cd python
python deeplabv3.py <pb_model> <TARGET_PLATFORM> <dtype(optional)> <output_rknn_path(optional)> <plot/save(optional)>
# such as: python deeplabv3.py ../model/deeplab-v3-plus-mobilenet-v2.pb rk3566
# output model will be saved as ../model/deeplab-v3-plus-mobilenet-v2.rknn
```
*Description:*

- <pb_model> should be the model path.
- <TARGET_PLATFORM> could be specified as RK3562, RK3566, RK3568, RK3588 according to board SOC version.
- <dtype\> is *optional*, could be specified as `i8` or `fp`, `i8` means to do quantization, `fp` means no to do quantization, default is `i8`.
- <output_rknn_path> is *optional*, used to specify the saving path of the RKNN model, default save path is `../model/deeplab-v3-plus-mobilenet-v2.rknn`
- <plot/save> is *optional*, `plot` means to display the segmentation results on the screen (without automatically saving the segmentation results), and `save` means to directly save the segmentation results. The segmentation results are displayed by default.

**Note**: Due to post-processing for reszie and argmax, the model needs to be cropped to run on the C demo. This is shown below by python scripts.

```py
rknn.load_tensorflow(ori_model, 
                        inputs=['sub_7'],
                        outputs=['logits/semantic/BiasAdd'],
                        input_size_list=[[1,513,513,3]])
```

Where `logits/semantic/BiasAdd` are selected as output node for deeplabv3 model rather than the original model output node.

## Expected Results

This example will print the segmentation result on the testing image, as follows:

![result](./reference_results/python_demo_result.png)



## Android Demo

### Compiling && Building

Modify the path of Android NDK in '[build-android.sh](../../build-android.sh)'.

For example,

```sh
ANDROID_NDK_PATH=~/opt/toolchain/android-ndk-r19c
```

Then, run this script:

```sh
# go back to the rknn_model_zoo root directory
# cd <rknn_model_zoo_path>
./build-android.sh -t <TARGET_PLATFORM> -a arm64-v8a -d deeplabv3
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board

Connect the USB port to PC, then push all demo files to the board.

For example, on RK3588,

```
adb root
adb remount
adb push install/rk3588_android_arm64-v8a/rknn_deeplabv3_demo/ /data/
```

### Running

```shell
adb shell

cd /data/rknn_deeplabv3_demo/lib/

# cp libOpenCL.so to lib
cp /vendor/lib64/libOpenCL.so .

# go to demo path
cd /data/rknn_deeplabv3_demo/


export LD_LIBRARY_PATH=./lib
./rknn_deeplabv3_demo ./model/deeplab-v3-plus-mobilenet-v2.rknn model/test_image.jpg
```
Note: The segmentation results will be saved in the `out.png`.



## Aarch64 Linux Demo

### Compiling && Building

According to the target platform, modify the path of 'GCC_COMPILER' in 'build-linux.sh'.

```sh
export GCC_COMPILER=/opt/tools/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu
```

Then, run the script:

```sh
./build-linux.sh  -t <TARGET_PLATFORM> -a aarch64 -d deeplabv3
```

Please use the specific platform instead of <TARGET_PLATFORM> above.

### Push all build output file to the board


Push install/<TARGET_PLATFORM>_linux_aarch64/rknn_deeplabv3_demo/ to the board.

- If use adb via the EVB board：

```shell
adb push install/<TARGET_PLATFORM>_linux_aarch64/rknn_deeplabv3_demo/ /data
```

- For other boards, use the scp or other different approaches to push all files under install/rknn_Deeplabv3_demo_<TARGET_PLATFORM>_linux_aarch64 to '/userdata'.

**Please use the specific platform instead of <TARGET_PLATFORM> above and make sure that there is a libOpenCL.so on the '/usr/lib' path or somewhere else on the board!**

### Running

```shell
adb shell
cd /data/rknn_deeplabv3_demo/

export LD_LIBRARY_PATH=./lib
./rknn_deeplabv3_demo model/deeplab-v3-plus-mobilenet-v2.rknn model/test_image.jpg
```

**For error message: can't find libOpenCL.so **

This error can be fixed by creating a soft link to ARM mali library **/usr/lib/aarch64-linux-gnu/libmali.so.1.9.0**

```shell
ln -s /usr/lib/aarch64-linux-gnu/libmali.so.1.9.0 libOpenCL.so
```

Then copy libOpenCL.so to path of your lib in demo and run it again.   
Note: **If the libmali.so cannot be found on the path of /usr/lib, please try to search this library in whole system or upgrade firmware to make sure there is a file named libmali.so on the board.**



## How to Modify C demo

### Rquirements

1. **The deeplabv3 C demo under the cpp folder only supports platforms of RK356x and RK3588 with GPU unit to run the postprocessing.**

**Note: if user wants to use other deeplabv3 model with differrnt size, please modify the following variables. **

```C++
// Modify it accroding to model io input
constexpr size_t OUT_SIZE = 65;  
constexpr size_t MASK_SIZE = 513;  
constexpr size_t NUM_LABEL = 21;  
```

Where the 'OUT_SIZE' refers to output size of your deeplabv3 model, 'MASK_SIZE' is the same as input size of your model.

**In addition to these variables, the cl kernel needs to be modified as shown below**.

```c
#define SRC_STRIDE 1365 //65*21 
```

**This variable must be modified according to your model output size multiplied with the number of label. It locates in file kernel_upsampleSoftmax.h in path_to_your_rknn_model_zoo/examples/Deeplabv3/cpp/gpu_postprocess/cl_kernels.**

## Expected Results

The expected result comes from C Demo.

![result](./reference_results/c_demo_result.png)

Note: Different platforms, different versions of tools and drivers may have slightly different results.
