#!/bin/bash

set -e

RV1109_TOOL_CHAIN="/home/xz/Documents/git_rk/compile_tools/1109/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf"


# for rv1109/rv1126 armhf
GCC_COMPILER=${RV1109_TOOL_CHAIN}/bin/arm-linux-gnueabihf

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
    -DCMAKE_C_COMPILER=${GCC_COMPILER}-gcc \
    -DCMAKE_CXX_COMPILER=${GCC_COMPILER}-g++ \
    -DMZ_ROOT=${MZ_ROOT} \
    -DTARGET_SOC=RV1109_1126 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DZERO_COPY=1
make -j4
make install
cd ..

cd ${BUILD_DIR}
cmake .. \
    -DCMAKE_C_COMPILER=${GCC_COMPILER}-gcc \
    -DCMAKE_CXX_COMPILER=${GCC_COMPILER}-g++ \
    -DMZ_ROOT=${MZ_ROOT} \
    -DTARGET_SOC=RV1109_1126 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DZERO_COPY=0
make -j4
make install

