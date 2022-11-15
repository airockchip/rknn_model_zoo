cmake_minimum_required(VERSION 3.4.1)

project(rknn_capi_test_linux)

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")


if (CMAKE_C_COMPILER MATCHES "aarch64")
  set(PLATFORM RK1808)
  set(LIB_ARCH lib64)
  add_definitions(-DRK1808=1)
else()
  set(PLATFORM RV1109_1126)
  set(LIB_ARCH lib)
  add_definitions(-DRK1808=0)
endif()


# stb
set(STB_DIR ${MZ_ROOT}/libs/common/)
include_directories(${STB_DIR})

# cnpy
set(CNPY_DIR ${MZ_ROOT}/libs/common/cnpy)
include_directories(${CNPY_DIR})

# zlib(used by libpng)
set(ZLIB_DIR ${MZ_ROOT}/libs/platform/${PLATFORM}/zlib)
include_directories(${ZLIB_DIR}/include)
set(ZLIB_LIB ${ZLIB_DIR}/lib/libz.a)

# drm
set(DRM_DIR ${MZ_ROOT}/libs/common/drm)
include_directories(${DRM_DIR}/include)
include_directories(${DRM_DIR}/include/libdrm)

# rknn api
set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu/rknn/rknn_api/librknn_api)
include_directories(${RKNN_API_PATH}/include)
set(RKNN_API_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknn_api.so)

# for 3399pro
# set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/RK3399Pro_npu/rknn-api/librknn_api)
# include_directories(${RKNN_API_PATH}/include)
# set(RKNN_API_LIB ${RKNN_API_PATH}/Linux/${LIB_ARCH}/librknn_api.so)

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
	)
endif()


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/${PLATFORM}/rknn_capi_test)
if (${ZERO_COPY})
	install(TARGETS rknn_capi_test_zero_copy DESTINATION ./)
else()
	install(TARGETS rknn_capi_test DESTINATION ./)
endif()

install(PROGRAMS ${RKNN_API_LIB} DESTINATION lib)
