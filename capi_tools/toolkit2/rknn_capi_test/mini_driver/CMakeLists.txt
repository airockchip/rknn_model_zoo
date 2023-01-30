cmake_minimum_required(VERSION 3.4.1)

project(rknn_capi_test)

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
  #set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../../runtime/RK356X/${CMAKE_SYSTEM_NAME}/librknn_api)
  #include_directories(${RKNN_API_PATH}/include)
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
#   set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../../runtime/RK3588/${CMAKE_SYSTEM_NAME}/librknn_api)
elseif(TARGET_SOC STREQUAL "rv110x")
  set(PLATFORM RV1106_1103)
  set(LIB_ARCH armhf)
  set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu2/runtime/RV1106/Linux/librknn_api)
  set(RKNN_RT_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknnmrt.so)
else()
  message(FATAL_ERROR "TARGET_SOC is not set, ref value: rk356x or rk3588 ")
endif()

include_directories(${RKNN_API_PATH}/include)


# CNPY
set(CNPY_DIR ${CMAKE_SOURCE_DIR}/cnpy)
include_directories(${CNPY_DIR})
message("CNPY_DIR: ${CNPY_DIR}")


# drm
set(DRM_DIR ${MZ_ROOT}/libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)


#stb CImg
set(STB_DIR ${MZ_ROOT}/libs/common/)
include_directories(${STB_DIR})

# zlib(used by libpng)
set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/libz.a)

string(REPLACE "-DANDROID" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-DANDROID" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_INSTALL_RPATH "lib")

include_directories( ${CMAKE_SOURCE_DIR}/include)

add_executable(rknn_capi_test_zero_copy_NC1HWC2
        src/rknn_api_native_output_test.c
        ${CNPY_DIR}/cnpy.h
        src/float16.h
)

target_link_libraries(rknn_capi_test_zero_copy_NC1HWC2
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
)

add_executable(rknn_capi_test_zero_copy_NCHW
        src/rknn_api_native_nhwc_output_test.c
        ${CNPY_DIR}/cnpy.h
)

target_link_libraries(rknn_capi_test_zero_copy_NCHW
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../install/${TARGET_SOC}/${CMAKE_SYSTEM_NAME}/rknn_capi_test)
install(TARGETS rknn_capi_test_zero_copy_NC1HWC2 DESTINATION ./)
install(TARGETS rknn_capi_test_zero_copy_NCHW DESTINATION ./)

install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
