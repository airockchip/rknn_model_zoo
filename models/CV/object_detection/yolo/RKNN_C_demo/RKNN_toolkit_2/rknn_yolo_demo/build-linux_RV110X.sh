set -e

TARGET_SOC="rv110x"

if [ -z $RK_RV1106_TOOLCHAIN ]; then
  echo "Please set the RK_RV1106_TOOLCHAIN environment variable!"
  echo "example:"
  echo "  export RK_RV1106_TOOLCHAIN=<path-to-your-dir/arm-rockchip830-linux-uclibcgnueabihf>"
  exit
fi

# for arm
GCC_COMPILER=${RK_RV1106_TOOLCHAIN}/bin/arm-rockchip830-linux-uclibcgnueabihf

export LD_LIBRARY_PATH=${TOOL_CHAIN}/lib64:$LD_LIBRARY_PATH
export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

MZ_ROOT=$(pwd | sed 's/\(rknn_model_zoo\).*/\1/g')

# build
BUILD_DIR=${ROOT_PWD}/build/build_linux_aarch64

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
cmake ../.. -DCMAKE_SYSTEM_NAME=Linux -DTARGET_SOC=${TARGET_SOC} -DMZ_ROOT=${MZ_ROOT}
make -j4
make install
cd -
