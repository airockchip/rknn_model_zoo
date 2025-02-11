// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <opencv2/opencv.hpp>
#include "preprocess.h"

#include "mobilesam.h"
#define MASK_THRESHOLD 0
#define COLOR (int[]){30, 144, 144}

int argmax(float* arr, int size)
{
    int index = 0;
    for (int i = 0; i < size; i++)
    {
        if (arr[i] > arr[index])
        {
            index = i;
        }
    }
    return index;
}

void resize_by_opencv_fp(float *input_image, int input_height, int input_width, float *output_image, int target_height, int target_width)
{
    cv::Mat src_image(input_height, input_width, CV_32F, input_image);
    cv::Mat dst_image;
    cv::resize(src_image, dst_image, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
    memcpy(output_image, dst_image.data, target_width * target_height * sizeof(float));
}

void slice_mask(float* ori_mask, float* slice_mask, int ori_width, int slice_height, int slice_width)
{
    for (int i = 0; i < slice_height; i++)
    {
        for (int j = 0; j < slice_width; j++)
        {
            slice_mask[i * slice_width + j] = ori_mask[i * ori_width + j];
        }
    }
}

void crop_mask(float* ori_mask, int size, uint8_t* res_mask, float threshold)
{
    for (int i = 0; i < size; i++)
    {
        res_mask[i] = ori_mask[i] > threshold ? 1 : 0; 
    }
}

int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

int post_process(rknn_app_context_t* app_ctx, float* iou_predictions, float* low_res_masks, mobilesam_res* res, int ori_height, int ori_width)
{
    int masks_num = app_ctx->decoder.output_attrs[0].n_elems;
    int index = argmax(iou_predictions, masks_num);

    float* mask_img_size = (float*)malloc(IMG_SIZE * IMG_SIZE * sizeof(float));
    int low_res_masks_height = app_ctx->decoder.output_attrs[1].dims[2];
    int low_res_masks_width = app_ctx->decoder.output_attrs[1].dims[3];
    resize_by_opencv_fp(low_res_masks + index * low_res_masks_height * low_res_masks_width, low_res_masks_height, low_res_masks_width, mask_img_size, IMG_SIZE, IMG_SIZE);

    int* new_shape = (int*)malloc(2 * sizeof(int));
    get_preprocess_shape(ori_height, ori_width, new_shape);

    float* mask_new_shape = (float*)malloc(new_shape[0] * new_shape[1] * sizeof(float));
    float* mask_ori_img = (float*)malloc(ori_height * ori_width * sizeof(float));
    slice_mask(mask_img_size, mask_new_shape, IMG_SIZE, new_shape[0], new_shape[1]);
    resize_by_opencv_fp(mask_new_shape, new_shape[0], new_shape[1], mask_ori_img, ori_height, ori_width);

    uint8_t* res_mask = (uint8_t*)malloc(ori_height * ori_width * sizeof(uint8_t));
    crop_mask(mask_ori_img, ori_height*ori_width, res_mask, MASK_THRESHOLD);

    res->mask = res_mask;
    res->score = iou_predictions[index];

    if (mask_img_size != NULL)
    {
        free(mask_img_size);
    }

    if (mask_new_shape != NULL)
    {
        free(mask_new_shape);
    }

    if (mask_ori_img != NULL)
    {
        free(mask_ori_img);
    }

    if (new_shape != NULL)
    {
        free(new_shape);
    }

    return 0;
}

void draw_mask(image_buffer_t* src_imag, uint8_t* mask)
{
    int width = src_imag->width;
    int height = src_imag->height;

    char* ori_img = (char *)src_imag->virt_addr;
    float alpha = 0.5f;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int pixel_offset = 3 * (i * width + j);
            if (mask[i * width + j] != 0)
            {
                ori_img[pixel_offset + 0] = (unsigned char)clamp(COLOR[0] * (1 - alpha) + ori_img[pixel_offset + 0] * alpha, 0, 255);
                ori_img[pixel_offset + 1] = (unsigned char)clamp(COLOR[1] * (1 - alpha) + ori_img[pixel_offset + 1] * alpha, 0, 255);
                ori_img[pixel_offset + 2] = (unsigned char)clamp(COLOR[2] * (1 - alpha) + ori_img[pixel_offset + 2] * alpha, 0, 255);
            }
        }
    }
}

void rknn_nchw_2_nhwc(float* nchw, float* nhwc, int N, int C, int H, int W)
{
    for (int ni = 0; ni < N; ni++)
    {
        for (int hi = 0; hi < H; hi++)
        {
            for (int wi = 0; wi < W; wi++)
            {
                for (int ci = 0; ci < C; ci++)
                {
                    memcpy(nhwc + ni * H * W * C + hi * W * C + wi * C + ci, nchw + ni * C * H * W + ci * H * W + hi * W + wi, sizeof(float));
                }
            }
        }
    }
}