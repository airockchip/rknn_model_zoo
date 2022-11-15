cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolov5_demo)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# install target and libraries
#set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_yolov5_demo_${CMAKE_SYSTEM_NAME})

set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


# rknn api
if(TARGET_SOC STREQUAL "rk356x")
  set(PLATFORM RK3566_3568)
  if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(LIB_ARCH arm64-v8a)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK356X/Android/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
  else()
    set(LIB_ARCH aarch64)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK356X/Linux/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
  endif()
elseif(TARGET_SOC STREQUAL "rk3588")
  set(PLATFORM RK3588)
  if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(LIB_ARCH arm64-v8a)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK3588/Android/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
  else()
    set(LIB_ARCH aarch64)
    set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RK3588/Linux/librknn_api)
    set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)
  endif()
else()
  message(FATAL_ERROR "TARGET_SOC is not set, ref value: rk356x or rk3588 ")
endif()


include_directories(${RKNN_API_PATH}/include)


# drm
set(DRM_DIR ${MZ_ROOT}/libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)


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

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
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



#rga
if (TARGET_SOC STREQUAL "rk356x" OR TARGET_SOC STREQUAL "rk3588")
  set(RGA_PATH ${MZ_ROOT}/libs/rklibs/librga)
else()
  message(FATAL_ERROR "TARGET_SOC is not set, ref value: rk356x or rk3588")
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(RGA_LIB ${RGA_PATH}/libs/AndroidNdk/${LIB_ARCH}/librga.so)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(RGA_LIB ${RGA_PATH}/libs/Linux/gcc-${LIB_ARCH}/librga.so)
endif()
include_directories(${RGA_PATH}/include)

string(REPLACE "-DANDROID" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-DANDROID" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_INSTALL_RPATH "lib")


# rknn_yolo_demo
include_directories( ${CMAKE_SOURCE_DIR}/include)

add_executable(rknn_yolo_demo
        src/drm_func.c
        src/rga_func.c
        src/yolo.cc
        src/main.cc
)

add_executable(rknn_yolo_zeroCopy_demo
        src/drm_func.c
        src/rga_func.c
        src/yolo.cc
        src/zero_copy_demo.cc
)

target_link_libraries(rknn_yolo_demo
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
)


target_link_libraries(rknn_yolo_zeroCopy_demo
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_yolo_demo_${CMAKE_SYSTEM_NAME})
install(TARGETS rknn_yolo_demo DESTINATION ./)
install(TARGETS rknn_yolo_zeroCopy_demo DESTINATION ./)

install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(PROGRAMS ${RGA_LIB} DESTINATION lib)
install(DIRECTORY model DESTINATION ./)
