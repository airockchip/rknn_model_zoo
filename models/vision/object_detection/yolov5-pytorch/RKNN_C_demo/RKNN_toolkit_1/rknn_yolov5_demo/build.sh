#!/bin/bash

set -e

RK1808_TOOL_CHAIN="/home/xz/Documents/git_rk/compile_tools/1808/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu"
RV1109_TOOL_CHAIN="/home/xz/Documents/git_rk/compile_tools/1109/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf"


# for rk1808 aarch64
GCC_COMPILER=${RK1808_TOOL_CHAIN}/bin/aarch64-linux-gnu

# for rk1806 armhf
# GCC_COMPILER=~/opts/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf

# for rv1109/rv1126 armhf
# GCC_COMPILER=${RV1109_TOOL_CHAIN}/bin/arm-linux-gnueabihf

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

# build rockx
BUILD_DIR=${ROOT_PWD}/build

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
cmake .. \
    -DCMAKE_C_COMPILER=${GCC_COMPILER}-gcc \
    -DCMAKE_CXX_COMPILER=${GCC_COMPILER}-g++
make -j4
make install
cd -