#ifndef _RKNN_DEMO_LPRNET_H_
#define _RKNN_DEMO_LPRNET_H_

#include "rknn_api.h"
#include "common.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#define MODEL_HEIGHT 24
#define MODEL_WIDTH 94
#define OUT_ROWS 68
#define OUT_COLS 18

#if defined(RV1106_1103)
#include "dma_alloc.hpp"
typedef struct
{
    char *dma_buf_virt_addr;
    int dma_buf_fd;
    int size;
} rknn_dma_buf;
#endif

typedef struct
{
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
#if defined(RV1106_1103)
    rknn_tensor_mem *input_mems[1];
    rknn_tensor_mem *output_mems[1];
    rknn_dma_buf img_dma_buf;
#endif
    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
} rknn_app_context_t;

typedef struct
{
    std::string plate_name;
} lprnet_result;

const std::vector<std::string>
    plate_code{
        "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑",
        "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤",
        "桂", "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁",
        "新",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "A", "B", "C", "D", "E", "F", "G", "H", "J", "K",
        "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V",
        "W", "X", "Y", "Z", "I", "O", "-"};

int init_lprnet_model(const char *model_path, rknn_app_context_t *app_ctx);

int release_lprnet_model(rknn_app_context_t *app_ctx);

int inference_lprnet_model(rknn_app_context_t *app_ctx, image_buffer_t *img, lprnet_result *out_result);

#endif //_RKNN_DEMO_LPRNET_H_