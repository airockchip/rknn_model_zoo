cmake_minimum_required(VERSION 3.10)

project(rknn_mobilesam_demo)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

#opencv
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
message(STATUS "64bit")
set (TARGET_LIB_ARCH lib64)
else()
message(STATUS "32bit")
set (TARGET_LIB_ARCH lib)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/opencv/opencv-android-sdk-build/sdk/native/jni/abi-${CMAKE_ANDROID_ARCH_ABI})
else()
    if(TARGET_LIB_ARCH STREQUAL "lib")
      set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/opencv/opencv-linux-armhf/share/OpenCV)
    else()
      set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/opencv/opencv-linux-aarch64/share/OpenCV)
    endif()
endif()
find_package(OpenCV REQUIRED)
message(STATUS OpenCV_DIR=${OpenCV_DIR})
message(STATUS OpenCV_LIBS=${OpenCV_LIBS})

set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

include_directories(${CMAKE_SOURCE_DIR}/rknpu2/rknn_mobilesam_utils)

set(rknn_mobilesam_utils rknpu2/rknn_mobilesam_utils/rknn_mobilesam_utils.cc)
set(mobilesam rknpu2/mobilesam.cc)

add_executable(${PROJECT_NAME}
    main.cc
	postprocess.cc
    preprocess.cc
    ${rknn_mobilesam_utils}
    ${mobilesam}
)

target_link_libraries(${PROJECT_NAME}
	imageutils
    imagedrawing
    fileutils
    ${LIBRKNNRT}
    ${OpenCV_LIBS}
    dl
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(${PROJECT_NAME}
    log
)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME}
    pthread
)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRKNNRT_INCLUDES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/coords.txt DESTINATION ./model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/labels.txt DESTINATION ./model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/picture.jpg DESTINATION ./model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)