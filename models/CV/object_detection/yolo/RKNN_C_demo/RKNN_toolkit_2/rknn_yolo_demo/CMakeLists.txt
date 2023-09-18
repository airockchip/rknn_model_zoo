cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolo_demo)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


# GENERAL LIB
# rknn api
if(TARGET_SOC STREQUAL "rk356x")
  set(PLATFORM RK3566_3568)
  if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(LIB_ARCH arm64-v8a)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK356X/Android/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/AndroidNdk/arm64-v8a/librga.a)
  else()
    set(LIB_ARCH aarch64)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK356X/Linux/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/Linux/gcc-aarch64/librga.a)
  endif()
elseif(TARGET_SOC STREQUAL "rk3588")
  set(PLATFORM RK3588)
  add_definitions(-DPLATFORM_RK3588)
  if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(LIB_ARCH arm64-v8a)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK3588/Android/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/AndroidNdk/arm64-v8a/librga.a)
  else()
    set(LIB_ARCH aarch64)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK3588/Linux/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/Linux/gcc-aarch64/librga.a)
  endif()
elseif(TARGET_SOC STREQUAL "rv110x")
  set(PLATFORM RV1106_1103)
  add_definitions(-DPLATFORM_RV1106_1103)
  set(LIB_ARCH armhf)
  set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RV1106/Linux/librknn_api)
  set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnmrt.so)
  set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/Linux/gcc-armhf/librga.a)
else()
  message(FATAL_ERROR "TARGET_SOC is not set, ref value: rk356x or rk3588 ")
endif()
include_directories(${RKNN_API_PATH}/include)


# drm
set(DRM_DIR ${MZ_ROOT}/libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)

# rga
include_directories(${MZ_ROOT}/libs/rklibs/librga/include)
include_directories(${MZ_ROOT}/libs/rklibs/librga/samples/utils/allocator/include)
set(RGA_DMA_ALLOC ${MZ_ROOT}/libs/rklibs/librga/samples/utils/allocator/dma_alloc.cpp)
# include(${MZ_ROOT}/libs/rklibs/librga/samples/utils/CMakeLists.txt)

#stb CImg
set(STB_DIR ${MZ_ROOT}/libs/common/)
include_directories(${STB_DIR})


if (CMAKE_SYSTEM_NAME STREQUAL "Android")
# libjpeg
  set(LIBJPEG_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/libjpeg)
  include_directories(${LIBJPEG_DIR}/include)
  set(LIBJPEG_LIB ${LIBJPEG_DIR}/lib/Android/libjpeg.a)
# libpng
  set(LIBPNG_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/libpng)
  include_directories(${LIBPNG_DIR}/include)
  set(LIBPNG_LIB ${LIBPNG_DIR}/lib/Android/libpng16.a)
# zlib(used by libpng)
  set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/zlib)
  include_directories(${ZLIB_DIR}/include)
  set(ZLIB_LIB ${ZLIB_DIR}/lib/Android/libz.a)
endif()

if ((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND (NOT (TARGET_SOC STREQUAL "rv110x")))
# libpng
  set(LIBPNG_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/libpng)
  include_directories(${LIBPNG_DIR}/include)
  set(LIBPNG_LIB ${LIBPNG_DIR}/lib/Linux/libpng16.a)
# libjpeg
  set(LIBJPEG_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/libjpeg)
  include_directories(${LIBJPEG_DIR}/include)
  set(LIBJPEG_LIB ${LIBJPEG_DIR}/lib/Linux/libjpeg.a)
# zlib(used by libpng)
  set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/zlib)
  include_directories(${ZLIB_DIR}/include)
  set(ZLIB_LIB ${ZLIB_DIR}/lib/Linux/libz.a)
endif()


string(REPLACE "-DANDROID" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-DANDROID" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_INSTALL_RPATH "lib")

# YOLO DEMO LIB
# rknn_yolo_demo
set(RKDEMO_UTILS ${MZ_ROOT}/libs/common/rkdemo_utils)
set(RKDEMO_UTILS_CC ${RKDEMO_UTILS}/rknn_demo_utils.cc)
include_directories( ${RKDEMO_UTILS})

include_directories( ${CMAKE_SOURCE_DIR}/include)
include_directories( ${CMAKE_SOURCE_DIR}/../../yolo_utils)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set (PTHREAD "")
else()
  set (PTHREAD pthread)
endif()

if (NOT (TARGET_SOC STREQUAL "rv110x"))
  add_executable(rknn_yolo_demo
          src/main.cc
          src/yolo.cc
          ../../yolo_utils/resize_function.cc
          ${RKDEMO_UTILS_CC}
          ${RGA_DMA_ALLOC}
          )
  
  target_link_libraries(rknn_yolo_demo
    ${RKNN_RT_LIB}
    ${RGA_LIB}
    ${LIBJPEG_LIB}
    ${LIBPNG_LIB}
    ${ZLIB_LIB}
    dl
    ${PTHREAD}
  )
endif()

if (TARGET_SOC STREQUAL "rv110x")
  add_definitions(-DRV110X_DEMO)
  add_definitions(-DRKDMO_NPU_2_NATIVE_ZP)
endif()

add_executable(rknn_yolo_demo_zero_copy
        src/yolo.cc
        src/zero_copy_demo.cc
        ../../yolo_utils/resize_function.cc
        ${RKDEMO_UTILS}/rknn_demo_utils.cc
        ${RGA_DMA_ALLOC}
)

target_link_libraries(rknn_yolo_demo_zero_copy
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
  ${PTHREAD}
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/${TARGET_SOC}/${CMAKE_SYSTEM_NAME}/rknn_yolo_demo)
if (NOT (TARGET_SOC STREQUAL "rv110x"))
  install(TARGETS rknn_yolo_demo DESTINATION ./)
endif()
install(TARGETS rknn_yolo_demo_zero_copy DESTINATION ./)

install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(PROGRAMS ${RGA_LIB} DESTINATION lib)
install(DIRECTORY model DESTINATION ./)
