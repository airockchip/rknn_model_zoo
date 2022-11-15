#!/bin/bash

set -e

if [ -z ${ANDROID_NDK_PATH} ]
then
  ANDROID_NDK_PATH=~/opt/android-ndk-r16b
fi

BUILD_TYPE=Release

TARGET_SOC="rk356x"

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

BUILD_DIR=${ROOT_PWD}/build/build_android_v8a

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
cmake ../.. \
        -DANDROID_TOOLCHAIN=clang \
        -DTARGET_SOC=${TARGET_SOC} \
       	-DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI="arm64-v8a" \
        -DANDROID_STL=c++_static \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
make -j4
make install
cd ..

