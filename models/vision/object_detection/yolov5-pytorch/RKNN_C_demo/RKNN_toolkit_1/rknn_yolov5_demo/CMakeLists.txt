cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolov5_demo_linux)

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if (CMAKE_C_COMPILER MATCHES "aarch64")
  set(PLATFORM RK1808)
  set(LIB_ARCH lib64)
else()
  set(PLATFORM RV1109_1126)
  set(LIB_ARCH lib)
endif()

# rga
set(RGA_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/${PLATFORM}/rga)
include_directories(${RGA_DIR}/include)

# drm
set(DRM_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)

#stb
set(STB_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/common/)
include_directories(${STB_DIR})
# include_directories(${CMAKE_SOURCE_DIR}/../../../../../../../libs/common/)

# rknn api
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../../../../../../../libs/)
include_directories(${RKNN_API_PATH}/${PLATFORM}/librknn_api/include)
set(RKNN_API_LIB ${RKNN_API_PATH}/${PLATFORM}/librknn_api/${LIB_ARCH}/librknn_api.so)

# libjpeg
set(LIBJPEG_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/${PLATFORM}/libjpeg)
include_directories(${LIBJPEG_DIR}/include)
set(LIBJPEG_LIB ${LIBJPEG_DIR}/lib/libjpeg.a)

# libpng
set(LIBPNG_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/${PLATFORM}/libpng)
include_directories(${LIBPNG_DIR}/include)
set(LIBPNG_LIB ${LIBPNG_DIR}/lib/libpng.a)

# zlib(used by libpng)
set(ZLIB_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../libs/${PLATFORM}/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/libz.a)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_INSTALL_RPATH "lib")

add_executable(rknn_yolov5_demo
        src/drm_func.c
        src/rga_func.c
        src/postprocess.cc
        src/main.cc
        )

target_link_libraries(rknn_yolov5_demo
	${RKNN_API_LIB}
  ${LIBJPEG_LIB}
  ${LIBPNG_LIB}
  ${ZLIB_LIB}
  dl
)

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_yolov5_demo)
install(TARGETS rknn_yolov5_demo DESTINATION ./)
install(DIRECTORY model DESTINATION ./)
install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
