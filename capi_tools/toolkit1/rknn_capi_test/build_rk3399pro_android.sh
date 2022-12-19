#!/bin/bash

set -e

# same as rk1808
ANDROID_NDK_PATH="/home/xz/Documents/git_rk/compile_tools/3568/android/android-ndk-r17"

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

# model_zoo_path root path
MZ_ROOT=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')

# build rockx
BUILD_DIR=${ROOT_PWD}/build

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi


cd ${BUILD_DIR}
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake \
    -DMZ_ROOT=${MZ_ROOT} \
    -DCMAKE_SYSTEM_NAME=Android \
    -DANDROID_ABI="arm64-v8a" \
    -DANDROID_PLATFORM=android-23 \
    -DANDROID_STL=c++_static \
    -DTARGET_SOC=RK3399PRO \
    -DZERO_COPY=0
make -j4
make install

