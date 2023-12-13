#ifndef _RKNN_DEMO_LPRNET_H_
#define _RKNN_DEMO_LPRNET_H_

#include "rknn_api.h"
#include "common.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

typedef struct
{
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    int model_channel;
    int model_width;
    int model_height;
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