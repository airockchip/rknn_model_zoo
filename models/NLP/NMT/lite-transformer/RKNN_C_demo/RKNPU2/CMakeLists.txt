cmake_minimum_required(VERSION 3.4.1)

project(rknn_lite_transformer_demo)

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# # skip 3rd-party lib dependencies
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_lite_transformer_demo_${CMAKE_SYSTEM_NAME})

set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


# rknn api
if(TARGET_SOC STREQUAL "rk356x")
  set(PLATFORM "RK356X")
elseif(TARGET_SOC STREQUAL "rk3588")
  set(PLATFORM "RK3588")
else()
  message(FATAL_ERROR "TARGET_SOC is not set, ref value: rk356x or rk3588 or rv110x")
endif()

set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/${PLATFORM}/${CMAKE_SYSTEM_NAME}/librknn_api)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(RKNN_RT_LIB ${RKNN_API_PATH}/${CMAKE_ANDROID_ARCH_ABI}/librknnrt.so)
else()
  if (CMAKE_C_COMPILER MATCHES "aarch64")
    set(LIB_ARCH aarch64)
  else()
    set(LIB_ARCH armhf)
  endif()
  set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
endif()

# nmt lib
set(RKDEMO_UTILS ${MZ_ROOT}/libs/common/rkdemo_utils)
set(RKDEMO_UTILS_CC ${RKDEMO_UTILS}/rknn_demo_utils.cc)
include_directories( ${RKDEMO_UTILS})

include_directories(${RKNN_API_PATH}/include)
include_directories(${CMAKE_SOURCE_DIR}/../bpe_tools)

set(CNPY_DIR ${MZ_ROOT}/libs/common/cnpy)
include_directories(${CNPY_DIR})

set(ZLIB_DIR ${MZ_ROOT}/libs/platform/RK3566_3568/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/${CMAKE_SYSTEM_NAME}/libz.a)

set(CMAKE_INSTALL_RPATH "lib")

# rknn_lite_transformer_demo
include_directories( ${CMAKE_SOURCE_DIR}/include)

add_executable(rknn_lite_transformer_demo
        # src/normal_api.cc
        src/zero_copy.cc
        # src/normal_api_weight_reuse.cc
        # src/zero_copy_weight_reuse.cc
        ../bpe_tools/bpe_tools.cc
        ${CNPY_DIR}/cnpy.cpp
        ${RKDEMO_UTILS_CC}
)

target_link_libraries(rknn_lite_transformer_demo
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${OpenCV_LIBS}
  ${ZLIB_LIB}
  dl
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_lite_transformer_demo_${CMAKE_SYSTEM_NAME})
install(TARGETS rknn_lite_transformer_demo DESTINATION ./)

install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(PROGRAMS ${RGA_LIB} DESTINATION lib)
install(DIRECTORY model DESTINATION ./)
