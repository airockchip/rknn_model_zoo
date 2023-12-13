cmake_minimum_required(VERSION 3.10)

project(rknn_lite_transformer_demo)

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


include_directories(${CMAKE_SOURCE_DIR}/utils)
include_directories(${CMAKE_SOURCE_DIR}/utils/cnpy)
include_directories(${CMAKE_SOURCE_DIR}/utils/bpe_tools)
include_directories(${CMAKE_SOURCE_DIR}/rknpu2/rkdemo_utils)

add_executable(${PROJECT_NAME}
    main.cc
    rknpu2/lite_transformer.cc
    rknpu2/rkdemo_utils/rknn_demo_utils.cc
    utils/bpe_tools/bpe_tools.cc
)

target_link_libraries(${PROJECT_NAME}
    fileutils
    ${LIBRKNNRT}
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
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/bpe_order.txt DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/dict_order.txt DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/cw_token_map_order.txt DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/position_embed.bin DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/token_embed.bin DESTINATION model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)