cmake_minimum_required(VERSION 3.10)

project(rknn_clip_demo)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

include_directories(${CMAKE_SOURCE_DIR}/tokenizer)
include_directories(${CMAKE_SOURCE_DIR}/rknpu2/rknn_clip_utils)

set(clip_file rknpu2/clip.cc)
set(rknn_clip_utils rknpu2/rknn_clip_utils/rknn_clip_utils.cc)
set(clip_tokenizer tokenizer/clip_tokenizer.cpp)

add_executable(${PROJECT_NAME}
    main.cc
	postprocess.cc
    ${clip_file}
    ${rknn_clip_utils}
	${clip_tokenizer}
)

target_link_libraries(${PROJECT_NAME}
	imageutils
    fileutils
    ${LIBRKNNRT}
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
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/text.txt DESTINATION ./model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/dog_224x224.jpg DESTINATION ./model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)