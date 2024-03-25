#!/bin/bash

set -e

if [[ -z ${ANDROID_NDK_PATH} ]];then
    echo "Please set ANDROID_NDK_PATH, such as ANDROID_NDK_PATH=~/opts/ndk/android-ndk-r18b"
    echo "NDK Version r18, r19 is recommanded. Other version may cause build failure."
    exit
fi

if [[ -e ${ANDROID_NDK_PATH}/source.properties ]];then
    ndk_version=`strings ${ANDROID_NDK_PATH}/source.properties |  grep -oE 'Revision = ([0-9]+)' | awk '{print $NF}'`
    # echo "NDK Version ${ndk_version}"
    if [ "$ndk_version" != "18" ] && [ "$ndk_version" != "19" ] && [ "$ndk_version" != "" ]; then
      #`"$ndk_version" != ""` used to avoid build script reporting error when it cannot read ndk_version
      echo "NDK Version ${ndk_version} is not recommanded. Please use 18 or 19"
      exit
    fi
fi

echo "$0 $@"
while getopts ":t:a:d:b:m" opt; do
  case $opt in
    t)
      TARGET_SOC=$OPTARG
      ;;
    a)
      TARGET_ARCH=$OPTARG
      ;;
    d)
      BUILD_DEMO_NAME=$OPTARG
      ;;
    b)
      BUILD_TYPE=$OPTARG
      ;;
    m)
      ENABLE_ASAN=ON
      export ENABLE_ASAN=TRUE
      ;;
    :)
      echo "Option -$OPTARG requires an argument." 
      exit 1
      ;;
    ?)
      echo "Invalid option: -$OPTARG index:$OPTIND"
      ;;
  esac
done

if [ -z ${TARGET_SOC} ]  || [ -z ${TARGET_ARCH} ] ||  [ -z ${BUILD_DEMO_NAME} ]; then
  echo "$0 -t <target> -a <arch> -d <build_demo_name> [-b <build_type>] [-m]"
  echo ""
  echo "    -t : target (rk356x/rk3588/rk3576)"
  echo "    -a : arch (arm64-v8a/armeabi-v7a)"
  echo "    -d : demo name"
  echo "    -b : build_type (Debug/Release)"
  echo "    -m : enable address sanitizer, build_type need set to Debug"
  echo "such as: $0  -t rk3588 -a arm64-v8a -d yolov5"
  echo ""
  exit -1
fi

# Debug / Release / RelWithDebInfo
if [[ -z ${BUILD_TYPE} ]];then
    BUILD_TYPE=Release
fi

# Build with Address Sanitizer for memory check, BUILD_TYPE need set to Debug
if [[ -z ${ENABLE_ASAN} ]];then
    ENABLE_ASAN=OFF
fi

for demo_path in `find examples -name "${BUILD_DEMO_NAME}"`
do
    if [ -d "$demo_path/cpp" ]
    then
        BUILD_DEMO_PATH="$demo_path/cpp"
        break;
    fi
done

if [[ -z "${BUILD_DEMO_PATH}" ]]
then
    echo "Cannot find demo: ${BUILD_DEMO_NAME}, only support:"

    for demo_path in `find examples -name cpp`
    do
        if [ -d "$demo_path" ]
        then
            dname=`dirname "$demo_path"`
            name=`basename $dname`
            echo "$name"
        fi
    done

    exit
fi

case ${TARGET_SOC} in
    rk356x)
        ;;
    rk3588)
        ;;
    rk3566)
        TARGET_SOC="rk356x"
        ;;
    rk3568)
        TARGET_SOC="rk356x"
        ;;
    rk3562)
        TARGET_SOC="rk356x"
        ;;
    rk3576)
        TARGET_SOC="rk3576"
        ;;
    *)
        echo "Invalid target: ${TARGET_SOC}"
        echo "Valid target: rk3562,rk3566,rk3568,rk3588,rk3576"
        exit -1
        ;;
esac

TARGET_SDK="rknn_${BUILD_DEMO_NAME}_demo"

TARGET_PLATFORM=${TARGET_SOC}_android
if [[ -n ${TARGET_ARCH} ]];then
TARGET_PLATFORM=${TARGET_PLATFORM}_${TARGET_ARCH}
fi
ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )
INSTALL_DIR=${ROOT_PWD}/install/${TARGET_PLATFORM}/${TARGET_SDK}
BUILD_DIR=${ROOT_PWD}/build/build_${TARGET_SDK}_${TARGET_PLATFORM}_${BUILD_TYPE}

echo "==================================="
echo "BUILD_DEMO_NAME=${BUILD_DEMO_NAME}"
echo "BUILD_DEMO_PATH=${BUILD_DEMO_PATH}"
echo "TARGET_SOC=${TARGET_SOC}"
echo "TARGET_ARCH=${TARGET_ARCH}"
echo "BUILD_TYPE=${BUILD_TYPE}"
echo "ENABLE_ASAN=${ENABLE_ASAN}"
echo "INSTALL_DIR=${INSTALL_DIR}"
echo "BUILD_DIR=${BUILD_DIR}"
echo "ANDROID_NDK_PATH=${ANDROID_NDK_PATH}"
echo "==================================="

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir -p ${BUILD_DIR}
fi

if [[ -d "${INSTALL_DIR}" ]]; then
  rm -rf ${INSTALL_DIR}
fi

cd ${BUILD_DIR}
cmake ../../${BUILD_DEMO_PATH} \
        -DTARGET_SOC=${TARGET_SOC} \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_SYSTEM_VERSION=23 \
        -DCMAKE_ANDROID_ARCH_ABI=${TARGET_ARCH} \
        -DCMAKE_ANDROID_STL_TYPE=c++_static \
        -DCMAKE_ANDROID_NDK=${ANDROID_NDK_PATH} \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DENABLE_ASAN=${ENABLE_ASAN} \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
# make VERBOSE=1
make -j4
make install

# Check if there is a rknn model in the install directory
suffix=".rknn"
shopt -s nullglob
if [ -d "$INSTALL_DIR" ]; then
    files=("$INSTALL_DIR/model/"/*"$suffix")
    shopt -u nullglob

    if [ ${#files[@]} -le 0 ]; then
        echo -e "\e[91mThe RKNN model can not be found in \"$INSTALL_DIR/model\", please check!\e[0m"
    fi
else
    echo -e "\e[91mInstall directory \"$INSTALL_DIR\" does not exist, please check!\e[0m"
fi
