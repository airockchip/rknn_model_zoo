set -e

TARGET_SOC="rv110x"

TOOL_CHAIN=~/opt/arm-rockchip830-linux-uclibcgnueabihf
GCC_COMPILER=${TOOL_CHAIN}/bin/arm-rockchip830-linux-uclibcgnueabihf

export LD_LIBRARY_PATH=${TOOL_CHAIN}/lib:$LD_LIBRARY_PATH
export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

MZ_ROOT=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')

# build
BUILD_DIR=${ROOT_PWD}/build/build_linux_rv110x

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
cmake ../../mini_driver -DCMAKE_SYSTEM_NAME=Linux -DTARGET_SOC=${TARGET_SOC} -DMZ_ROOT=${MZ_ROOT}
make -j4
make install
cd -
