cmake_minimum_required(VERSION 3.5)
enable_testing()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

####################
#  Project
project(gpu_postprocess)
set(OPENCL_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../3rdparty/opencl/libopencl-stub/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/base64)
include_directories(${OPENCL_PATH}/include)


#add_definitions(-Wall -Wextra -Wno-unused-function -fPIC)
add_definitions(-fPIC)
add_definitions(-DCL_TARGET_OPENCL_VERSION=120)

####################
#  Language
set(CMAKE_C_FLAGS_DEBUG            "${CMAKE_C_FLAGS_DEBUG} -O0 -g"          CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
set(CMAKE_C_FLAGS_RELEASE          "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG -s"     CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")

set(STDC_HEADERS 1)
set(CMAKE_CXX_STANDARD 14)
set(HAVE_CXX14 TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g"          CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -s"     CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -DNDEBUG -g3" CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds.")
option(ENABLE_WERROR "Compile with -Werror enabled (default is OFF)" OFF)
option(ENABLE_ASAN "Compile with address santitize (default is OFF)" OFF)
if (ENABLE_WERROR)
    add_compile_options(-Werror)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "CMAKE_BUILD_TYPE is Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -O3 ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -O3 ")
endif()


if (ENABLE_ASAN)
    message(STATUS "ENABLE ADDRESS SANTITIZE")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

####################
### opencl wrapper
set(libopencl_SRCS
    ${OPENCL_PATH}/src/libopencl.cc
)

add_library(OpenCL SHARED ${libopencl_SRCS})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(OpenCL "-Wl,--allow-shlib-undefined" dl)
endif()

####################
## gpu_postprocess
set(gpu_postprocessSRC  gpu_compose_impl.cc
                    workflow.cc
                    opencl_workspace.cc
                    opencl_memory.cc
                    opencl_wrapper.cc)
                
add_library(gpu_postprocess STATIC ${gpu_postprocessSRC})
target_link_libraries(gpu_postprocess "-Wl,--allow-shlib-undefined" ${CMAKE_CURRENT_BINARY_DIR}/libOpenCL.so dl)
