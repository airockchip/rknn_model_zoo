cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolo_demo_linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  # only for android rk3399pro
  set(LIB_ARCH lib64)
  set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/AndroidNdk/arm64-v8a/librga.a)
  set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/RK3399Pro_npu/rknn-api/librknn_api)
  include_directories(${RKNN_API_PATH}/include)
  set(RKNN_API_LIB ${RKNN_API_PATH}/${CMAKE_SYSTEM_NAME}/${LIB_ARCH}/librknn_api.so)
  add_definitions(-DRK3399PRO)
else()
  if (CMAKE_C_COMPILER MATCHES "aarch64")
    set(LIB_ARCH lib64)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/Linux/gcc-aarch64/librga.a)
  else()
    set(LIB_ARCH lib)
    set(RGA_LIB ${MZ_ROOT}/libs/rklibs/librga/libs/Linux/gcc-armhf/librga.a)
  endif()

  if (${TARGET_SOC} STREQUAL "RK3399PRO")
    add_definitions(-DRK3399PRO)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/RK3399Pro_npu/rknn-api/librknn_api)
    include_directories(${RKNN_API_PATH}/include)
    set(RKNN_API_LIB ${RKNN_API_PATH}/${CMAKE_SYSTEM_NAME}/${LIB_ARCH}/librknn_api.so)
  else()
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu/rknn/rknn_api/librknn_api)
    include_directories(${RKNN_API_PATH}/include)
    set(RKNN_API_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknn_api.so)
  endif()
endif()


#stb
set(STB_DIR ${MZ_ROOT}/libs/common/)
include_directories(${STB_DIR})
# libjpeg
set(LIBJPEG_DIR ${MZ_ROOT}/libs/platform/${TARGET_SOC}/libjpeg)
include_directories(${LIBJPEG_DIR}/include)
set(LIBJPEG_LIB ${LIBJPEG_DIR}/lib/${CMAKE_SYSTEM_NAME}/libjpeg.a)

# libpng
set(LIBPNG_DIR ${MZ_ROOT}/libs/platform/${TARGET_SOC}/libpng)
include_directories(${LIBPNG_DIR}/include)
set(LIBPNG_LIB ${LIBPNG_DIR}/lib/${CMAKE_SYSTEM_NAME}/libpng.a)

# zlib(used by libpng)
set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${TARGET_SOC}/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/${CMAKE_SYSTEM_NAME}/libz.a)

# rga
include_directories(${MZ_ROOT}/libs/rklibs/librga/include)
include_directories(${MZ_ROOT}/libs/rklibs/librga/samples/utils/allocator/include)
set(RGA_DMA_ALLOC ${MZ_ROOT}/libs/rklibs/librga/samples/utils/allocator/dma_alloc.cpp)
# include(${MZ_ROOT}/libs/rklibs/librga/samples/utils/CMakeLists.txt)


# YOLO DEMO LIB
set(RKDEMO_UTILS ${MZ_ROOT}/libs/common/rkdemo_utils)
set(RKDEMO_UTILS_CC ${RKDEMO_UTILS}/rknn_demo_utils.cc)
include_directories( ${RKDEMO_UTILS})
add_definitions(-DRKDMO_NPU_1)

include_directories( ${CMAKE_SOURCE_DIR}/include)
include_directories( ${CMAKE_SOURCE_DIR}/../../yolo_utils)

set(CMAKE_INSTALL_RPATH "lib")

add_executable(rknn_yolo_demo
        src/main.cc
        src/yolo.cc
        ../../yolo_utils/resize_function.cc
        ${RKDEMO_UTILS_CC}
        ${RGA_DMA_ALLOC}
        )

target_link_libraries(rknn_yolo_demo
	${RKNN_API_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  ${RGA_LIB}
  dl
)

if (NOT ${TARGET_SOC} STREQUAL "RK3399PRO")
  add_executable(rknn_yolo_demo_zero_copy
          src/main_zero_copy.cc
          src/yolo.cc
          ../../yolo_utils/resize_function.cc
          ${RKDEMO_UTILS_CC}
          ${RGA_DMA_ALLOC}
          )

  target_link_libraries(rknn_yolo_demo_zero_copy
    ${RKNN_API_LIB}
    ${LIBJPEG_LIB}
    ${LIBPNG_LIB}
    ${ZLIB_LIB}
    ${RGA_LIB}
    dl
  )
endif()

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/${TARGET_SOC}/${CMAKE_SYSTEM_NAME}/rknn_yolo_demo)
install(TARGETS rknn_yolo_demo DESTINATION ./)

if (NOT ${TARGET_SOC} STREQUAL "RK3399PRO")
install(TARGETS rknn_yolo_demo_zero_copy DESTINATION ./)
endif()

install(DIRECTORY model DESTINATION ./)
install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
