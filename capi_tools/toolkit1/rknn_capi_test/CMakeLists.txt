cmake_minimum_required(VERSION 3.4.1)

project(rknn_capi_test_linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# for linux
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  if (CMAKE_C_COMPILER MATCHES "aarch64")
    set(LIB_ARCH lib64)
    add_definitions(-DRK1808=1)
  else()
    set(LIB_ARCH lib)
    add_definitions(-DRK1808=0)
  endif()
  set(PTHREAD pthread)
else()
  set(LIB_ARCH lib64)
  add_definitions(-DRK1808=1)
  set(PTHREAD "")
endif()
# for android
# if (${TARGET_SOC} STREQUAL "RK3399PRO")
#   set(LIB_ARCH lib64)
#   add_definitions(-DRK1808=1)
# endif()


# stb
set(STB_DIR ${MZ_ROOT}/libs/common/)
include_directories(${STB_DIR})

# cnpy
set(CNPY_DIR ${MZ_ROOT}/libs/common/cnpy)
include_directories(${CNPY_DIR})

# zlib(used by libpng)
set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${TARGET_SOC}/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/${CMAKE_SYSTEM_NAME}/libz.a)

# drm
set(DRM_DIR ${MZ_ROOT}/libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)

# rknn api
if (${TARGET_SOC} STREQUAL "RK3399PRO")
  set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/RK3399Pro_npu/rknn-api/librknn_api)
  include_directories(${RKNN_API_PATH}/include)
  set(RKNN_API_LIB ${RKNN_API_PATH}/${CMAKE_SYSTEM_NAME}/${LIB_ARCH}/librknn_api.so)
else()
  set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu/rknn/rknn_api/librknn_api)
  include_directories(${RKNN_API_PATH}/include)
  set(RKNN_API_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknn_api.so)
endif()

# others
include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_INSTALL_RPATH "lib")

# message("ZERO_COPY:")
# message(${ZERO_COPY})
if (${ZERO_COPY})
	message("Build rknn_capi_test_zero_copy")
	add_definitions(-DUSE_ZERO_COPY=1)
	add_executable(rknn_capi_test_zero_copy
			src/main.cc
			src/drm_func.c
			${CNPY_DIR}/cnpy.cpp
	)

	target_link_libraries(rknn_capi_test_zero_copy
			${RKNN_API_LIB}
			# ${LIBJPEG_LIB}
			# ${LIBPNG_LIB}
			${ZLIB_LIB}
			dl
			${PTHREAD}
	)
else()
	message("Build rknn_capi_test")
	add_definitions(-DUSE_ZERO_COPY=0)
	add_executable(rknn_capi_test
			src/main.cc
			src/drm_func.c
			${CNPY_DIR}/cnpy.cpp
	)

	target_link_libraries(rknn_capi_test
			${RKNN_API_LIB}
			# ${LIBJPEG_LIB}
			# ${LIBPNG_LIB}
			${ZLIB_LIB}
			dl
			${PTHREAD}
	)
endif()


# install target and libraries

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/${TARGET_SOC}/${CMAKE_SYSTEM_NAME}/rknn_capi_test)


if (${ZERO_COPY})
	install(TARGETS rknn_capi_test_zero_copy DESTINATION ./)
else()
	install(TARGETS rknn_capi_test DESTINATION ./)
endif()

install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
