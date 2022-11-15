cmake_minimum_required(VERSION 3.4.1)

project(rknn_lite_transformer_demo)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
# skip 3rd-party lib dependencies
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_lite_transformer_demo_${CMAKE_SYSTEM_NAME})

set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# rknn api
if (CMAKE_C_COMPILER MATCHES "aarch64")
  set(PLATFORM RK1808)
  set(LIB_ARCH lib64)
else()
  set(PLATFORM RV1109_1126)
  set(LIB_ARCH lib)
endif()


set(RKNN_API_PATH ${MZ_ROOT}/libs/rklibs/rknpu/rknn/rknn_api/librknn_api)
include_directories(${RKNN_API_PATH}/include)
set(RKNN_API_LIB ${RKNN_API_PATH}/${LIB_ARCH}/librknn_api.so)
message( "RKNN_API_PATH " ${RKNN_API_PATH})
message( "RKNN_API_LIB " ${RKNN_API_LIB})

set(CMAKE_INSTALL_RPATH "lib")

# rknn_lite_transformer_demo
include_directories( ${CMAKE_SOURCE_DIR}/include)

add_executable(rknn_lite_transformer_demo
        src/main.cc
)

target_link_libraries(rknn_lite_transformer_demo
  ${RKNN_API_LIB}
  dl
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_lite_transformer_demo_${CMAKE_SYSTEM_NAME})
install(TARGETS rknn_lite_transformer_demo DESTINATION ./)

install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(PROGRAMS ${RGA_LIB} DESTINATION lib)
install(DIRECTORY model DESTINATION ./)
