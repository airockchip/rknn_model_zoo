// Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "yolov8_seg.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "rknn_matmul_api.h"
#include "im2d.hpp"
#include "dma_alloc.hpp"
#include "drm_alloc.hpp"
#include "Float16.h"
#include "easy_timer.h"

#include <set>
#include <vector>
#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"
// #define USE_FP_RESIZE

static char *labels[OBJ_CLASS_NUM];

int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;

    if (file == NULL)
    {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

static int loadLabelName(const char *locationFilename, char *label[])
{
    printf("load lable %s\n", locationFilename);
    readLines(locationFilename, label, OBJ_CLASS_NUM);
    return 0;
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                              float ymax1)
{
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
               int filterId, float threshold)
{
    for (int i = 0; i < validCount; ++i)
    {
        if (order[i] == -1 || classIds[i] != filterId)
        {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1 || classIds[i] != filterId)
            {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right)
    {
        key_index = indices[left];
        key = input[left];
        while (low < high)
        {
            while (low < high && input[high] <= key)
            {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key)
            {
                low++;
            }
            input[high] = input[low];
            indices[high] = indices[low];
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}

void resize_by_opencv_fp(float *input_image, int input_width, int input_height, int boxes_num, float *output_image, int target_width, int target_height)
{
    for (int b = 0; b < boxes_num; b++)
    {
        cv::Mat src_image(input_height, input_width, CV_32F, &input_image[b * input_width * input_height]);
        cv::Mat dst_image;
        cv::resize(src_image, dst_image, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
        memcpy(&output_image[b * target_width * target_height], dst_image.data, target_width * target_height * sizeof(float));
    }
}

void resize_by_opencv_uint8(uint8_t *input_image, int input_width, int input_height, int boxes_num, uint8_t *output_image, int target_width, int target_height)
{
    for (int b = 0; b < boxes_num; b++)
    {
        cv::Mat src_image(input_height, input_width, CV_8U, &input_image[b * input_width * input_height]);
        cv::Mat dst_image;
        cv::resize(src_image, dst_image, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
        memcpy(&output_image[b * target_width * target_height], dst_image.data, target_width * target_height * sizeof(uint8_t));
    }
}

void resize_by_rga_rk3588(uint8_t *input_image, int input_width, int input_height, uint8_t *output_image, int target_width, int target_height)
{
    char *src_buf, *dst_buf;
    int src_buf_size, dst_buf_size;
    rga_buffer_handle_t src_handle, dst_handle;
    int src_width = input_width;
    int src_height = input_height;
    int src_format = RK_FORMAT_YCbCr_400;
    int dst_width = target_width;
    int dst_height = target_height;
    int dst_format = RK_FORMAT_YCbCr_400;
    int dst_dma_fd, src_dma_fd;
    rga_buffer_t dst = {};
    rga_buffer_t src = {};

    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);
    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);

    /*
     * Allocate dma_buf within 4G from dma32_heap,
     * return dma_fd and virtual address.
     */
    dma_buf_alloc(DMA_HEAP_DMA32_UNCACHE_PATCH, dst_buf_size, &dst_dma_fd, (void **)&dst_buf);
    dma_buf_alloc(DMA_HEAP_DMA32_UNCACHE_PATCH, src_buf_size, &src_dma_fd, (void **)&src_buf);
    memcpy(src_buf, input_image, src_buf_size);

    dst_handle = importbuffer_fd(dst_dma_fd, dst_buf_size);
    src_handle = importbuffer_fd(src_dma_fd, src_buf_size);

    dst = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
    src = wrapbuffer_handle(src_handle, src_width, src_height, src_format);

    int ret = imresize(src, dst);
    if (ret == IM_STATUS_SUCCESS)
    {
        printf("%s running success!\n", "rga_resize");
    }
    else
    {
        printf("%s running failed, %s\n", "rga_resize", imStrError((IM_STATUS)ret));
    }

    memcpy(output_image, dst_buf, target_width * target_height);

    releasebuffer_handle(src_handle);
    releasebuffer_handle(dst_handle);
    dma_buf_free(src_buf_size, &src_dma_fd, src_buf);
    dma_buf_free(dst_buf_size, &dst_dma_fd, dst_buf);
}

class DrmObject
{
public:
    int drm_buffer_fd;
    int drm_buffer_handle;
    size_t actual_size;
    uint8_t *drm_buf;
};

void resize_by_rga_rk356x(uint8_t *input_image, int input_width, int input_height, uint8_t *output_image, int target_width, int target_height)
{
    rga_buffer_handle_t src_handle, dst_handle;
    int src_width = input_width;
    int src_height = input_height;
    int src_format = RK_FORMAT_YCbCr_400;
    int dst_width = target_width;
    int dst_height = target_height;
    int dst_format = RK_FORMAT_YCbCr_400;
    rga_buffer_t dst = {};
    rga_buffer_t src = {};
    DrmObject drm_src;
    DrmObject drm_dst;
    int src_alloc_flags = 0, dst_alloc_flags = 0;

    /* Allocate drm_buf, return dma_fd and virtual address. */
    drm_src.drm_buf = (uint8_t *)drm_buf_alloc(src_width, src_height,
                                               get_bpp_from_format(src_format) * 8,
                                               &drm_src.drm_buffer_fd,
                                               &drm_src.drm_buffer_handle,
                                               &drm_src.actual_size,
                                               src_alloc_flags);

    drm_dst.drm_buf = (uint8_t *)drm_buf_alloc(dst_width, dst_height,
                                               get_bpp_from_format(dst_format) * 8,
                                               &drm_dst.drm_buffer_fd,
                                               &drm_dst.drm_buffer_handle,
                                               &drm_dst.actual_size,
                                               dst_alloc_flags);
    memcpy(drm_src.drm_buf, input_image, drm_src.actual_size);

    src_handle = importbuffer_fd(drm_src.drm_buffer_fd, drm_src.actual_size);
    dst_handle = importbuffer_fd(drm_dst.drm_buffer_fd, drm_dst.actual_size);

    dst = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
    src = wrapbuffer_handle(src_handle, src_width, src_height, src_format);

    int ret = imresize(src, dst);
    if (ret == IM_STATUS_SUCCESS)
    {
        printf("%s running success!\n", "rga_resize");
    }
    else
    {
        printf("%s running failed, %s\n", "rga_resize", imStrError((IM_STATUS)ret));
    }

    memcpy(output_image, drm_dst.drm_buf, target_width * target_height);

    releasebuffer_handle(src_handle);
    releasebuffer_handle(dst_handle);
    drm_buf_destroy(drm_src.drm_buffer_fd, drm_src.drm_buffer_handle, drm_src.drm_buf, drm_src.actual_size);
    drm_buf_destroy(drm_dst.drm_buffer_fd, drm_dst.drm_buffer_handle, drm_dst.drm_buf, drm_dst.actual_size);
}

void crop_mask_fp(float *seg_mask, uint8_t *all_mask_in_one, float *boxes, int boxes_num, int *cls_id, int height, int width)
{
    for (int b = 0; b < boxes_num; b++)
    {
        float x1 = boxes[b * 4 + 0];
        float y1 = boxes[b * 4 + 1];
        float x2 = boxes[b * 4 + 2];
        float y2 = boxes[b * 4 + 3];

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                if (j >= x1 && j < x2 && i >= y1 && i < y2)
                {
                    if (all_mask_in_one[i * width + j] == 0)
                    {
                        if (seg_mask[b * width * height + i * width + j] > 0)
                        {
                            all_mask_in_one[i * width + j] = (cls_id[b] + 1);
                        }
                        else
                        {
                            all_mask_in_one[i * width + j] = 0;
                        }
                    }
                }
            }
        }
    }
}

void crop_mask_uint8(uint8_t *seg_mask, uint8_t *all_mask_in_one, float *boxes, int boxes_num, int *cls_id, int height, int width)
{
    for (int b = 0; b < boxes_num; b++)
    {
        float x1 = boxes[b * 4 + 0];
        float y1 = boxes[b * 4 + 1];
        float x2 = boxes[b * 4 + 2];
        float y2 = boxes[b * 4 + 3];

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                if (j >= x1 && j < x2 && i >= y1 && i < y2)
                {
                    if (all_mask_in_one[i * width + j] == 0)
                    {
                        if (seg_mask[b * width * height + i * width + j] > 0)
                        {
                            all_mask_in_one[i * width + j] = (cls_id[b] + 1);
                        }
                        else
                        {
                            all_mask_in_one[i * width + j] = 0;
                        }
                    }
                }
            }
        }
    }
}

void matmul_by_cpu_fp(std::vector<float> &A, float *B, float *C, int ROWS_A, int COLS_A, int COLS_B)
{

    float temp = 0;
    for (int i = 0; i < ROWS_A; i++)
    {
        for (int j = 0; j < COLS_B; j++)
        {
            temp = 0;
            for (int k = 0; k < COLS_A; k++)
            {
                temp += A[i * COLS_A + k] * B[k * COLS_B + j];
            }
            C[i * COLS_B + j] = temp;
        }
    }
}

void matmul_by_cpu_uint8(std::vector<float> &A, float *B, uint8_t *C, int ROWS_A, int COLS_A, int COLS_B)
{

    float temp = 0;
    for (int i = 0; i < ROWS_A; i++)
    {
        for (int j = 0; j < COLS_B; j++)
        {
            temp = 0;
            for (int k = 0; k < COLS_A; k++)
            {
                temp += A[i * COLS_A + k] * B[k * COLS_B + j];
            }
            if (temp > 0)
            {
                C[i * COLS_B + j] = 4;
            }
            else
            {
                C[i * COLS_B + j] = 0;
            }
        }
    }
}

void matmul_by_npu_fp(std::vector<float> &A_input, float *B_input, float *C_input, int ROWS_A, int COLS_A, int COLS_B, rknn_app_context_t *app_ctx)
{
    int B_layout = 0;
    int AC_layout = 0;
    int32_t M = ROWS_A;
    int32_t K = COLS_A;
    int32_t N = COLS_B;

    rknn_matmul_ctx ctx;
    rknn_matmul_info info;
    memset(&info, 0, sizeof(rknn_matmul_info));
    info.M = M;
    info.K = K;
    info.N = N;
    info.type = RKNN_FLOAT16_MM_FLOAT16_TO_FLOAT32;
    info.B_layout = B_layout;
    info.AC_layout = AC_layout;

    rknn_matmul_io_attr io_attr;
    memset(&io_attr, 0, sizeof(rknn_matmul_io_attr));

    rknpu2::float16 int8Vector_A[ROWS_A * COLS_A];
    for (int i = 0; i < ROWS_A * COLS_A; ++i)
    {
        int8Vector_A[i] = (rknpu2::float16)A_input[i];
    }

    rknpu2::float16 int8Vector_B[COLS_A * COLS_B];
    for (int i = 0; i < COLS_A * COLS_B; ++i)
    {
        int8Vector_B[i] = (rknpu2::float16)B_input[i];
    }

    int ret = rknn_matmul_create(&ctx, &info, &io_attr);
    // Create A
    rknn_tensor_mem *A = rknn_create_mem(ctx, io_attr.A.size);
    // Create B
    rknn_tensor_mem *B = rknn_create_mem(ctx, io_attr.B.size);
    // Create C
    rknn_tensor_mem *C = rknn_create_mem(ctx, io_attr.C.size);

    memcpy(A->virt_addr, int8Vector_A, A->size);
    memcpy(B->virt_addr, int8Vector_B, B->size);

    // Set A
    ret = rknn_matmul_set_io_mem(ctx, A, &io_attr.A);
    // Set B
    ret = rknn_matmul_set_io_mem(ctx, B, &io_attr.B);
    // Set C
    ret = rknn_matmul_set_io_mem(ctx, C, &io_attr.C);

    // Run
    ret = rknn_matmul_run(ctx);
    for (int i = 0; i < ROWS_A * COLS_B; ++i)
    {
        C_input[i] = ((float *)C->virt_addr)[i];
    }

    // destroy
    rknn_destroy_mem(ctx, A);
    rknn_destroy_mem(ctx, B);
    rknn_destroy_mem(ctx, C);
    rknn_matmul_destroy(ctx);
}

void seg_reverse(uint8_t *seg_mask, uint8_t *cropped_seg, uint8_t *seg_mask_real,
                 int model_in_height, int model_in_width, int cropped_height, int cropped_width, int ori_in_height, int ori_in_width, int y_pad, int x_pad)
{

    if (y_pad == 0 && x_pad == 0 && ori_in_height == model_in_height && ori_in_width == model_in_width)
    {
        memcpy(seg_mask_real, seg_mask, ori_in_height * ori_in_width);
        return;
    }

    int cropped_index = 0;
    for (int i = 0; i < model_in_height; i++)
    {
        for (int j = 0; j < model_in_width; j++)
        {
            if (i >= y_pad && i < model_in_height - y_pad && j >= x_pad && j < model_in_width - x_pad)
            {
                int seg_index = i * model_in_width + j;
                cropped_seg[cropped_index] = seg_mask[seg_index];
                cropped_index++;
            }
        }
    }
    // Note: Here are different methods provided for implementing single-channel image scaling.
    //       The method of using rga to resize the image requires that the image size is 2 aligned.
    resize_by_opencv_uint8(cropped_seg, cropped_width, cropped_height, 1, seg_mask_real, ori_in_width, ori_in_height);
    // resize_by_rga_rk356x(cropped_seg, cropped_width, cropped_height, seg_mask_real, ori_in_width, ori_in_height);
    // resize_by_rga_rk3588(cropped_seg, cropped_width, cropped_height, seg_mask_real, ori_in_width, ori_in_height);
}

static int box_reverse(int position, int boundary, int pad, float scale)
{
    return (int)((clamp(position, 0, boundary) - pad) / scale);
}

static float sigmoid(float x) { return 1.0 / (1.0 + expf(-x)); }

static float unsigmoid(float y) { return -1.0 * logf((1.0 / y) - 1.0); }

inline static int32_t __clip(float val, float min, float max)
{
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    int8_t res = (int8_t)__clip(dst_val, -128, 127);
    return res;
}

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

static void compute_dfl(float *tensor, int dfl_len, float *box)
{
    for (int b = 0; b < 4; b++)
    {
        float exp_t[dfl_len];
        float exp_sum = 0;
        float acc_sum = 0;
        for (int i = 0; i < dfl_len; i++)
        {
            exp_t[i] = exp(tensor[i + b * dfl_len]);
            exp_sum += exp_t[i];
        }

        for (int i = 0; i < dfl_len; i++)
        {
            acc_sum += exp_t[i] / exp_sum * i;
        }
        box[b] = acc_sum;
    }
}

static int process_i8(rknn_output *all_input, int input_id, int grid_h, int grid_w, int height, int width, int stride, int dfl_len,
                      std::vector<float> &boxes, std::vector<float> &segments, float *proto, std::vector<float> &objProbs, std::vector<int> &classId, float threshold,
                      rknn_app_context_t *app_ctx)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;

    // Skip if input_id is not 0, 4, 8, or 12
    if (input_id % 4 != 0)
    {
        return validCount;
    }

    if (input_id == 12)
    {
        int8_t *input_proto = (int8_t *)all_input[input_id].buf;
        int32_t zp_proto = app_ctx->output_attrs[input_id].zp;
        float scale_proto = app_ctx->output_attrs[input_id].scale;
        for (int i = 0; i < PROTO_CHANNEL * PROTO_HEIGHT * PROTO_WEIGHT; i++)
        {
            proto[i] = deqnt_affine_to_f32(input_proto[i], zp_proto, scale_proto);
        }
        return validCount;
    }

    int8_t *box_tensor = (int8_t *)all_input[input_id].buf;
    int32_t box_zp = app_ctx->output_attrs[input_id].zp;
    float box_scale = app_ctx->output_attrs[input_id].scale;

    int8_t *score_tensor = (int8_t *)all_input[input_id + 1].buf;
    int32_t score_zp = app_ctx->output_attrs[input_id + 1].zp;
    float score_scale = app_ctx->output_attrs[input_id + 1].scale;

    int8_t *score_sum_tensor = nullptr;
    int32_t score_sum_zp = 0;
    float score_sum_scale = 1.0;
    score_sum_tensor = (int8_t *)all_input[input_id + 2].buf;
    score_sum_zp = app_ctx->output_attrs[input_id + 2].zp;
    score_sum_scale = app_ctx->output_attrs[input_id + 2].scale;

    int8_t *seg_tensor = (int8_t *)all_input[input_id + 3].buf;
    int32_t seg_zp = app_ctx->output_attrs[input_id + 3].zp;
    float seg_scale = app_ctx->output_attrs[input_id + 3].scale;

    int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
    int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            int offset_seg = i * grid_w + j;
            int8_t *in_ptr_seg = seg_tensor + offset_seg;

            // for quick filtering through "score sum"
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < score_sum_thres_i8)
                {
                    continue;
                }
            }

            int8_t max_score = -score_zp;
            for (int c = 0; c < OBJ_CLASS_NUM; c++)
            {
                if ((score_tensor[offset] > score_thres_i8) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > score_thres_i8)
            {

                for (int k = 0; k < PROTO_CHANNEL; k++)
                {
                    float seg_element_fp = deqnt_affine_to_f32(in_ptr_seg[(k)*grid_len], seg_zp, seg_scale);
                    segments.push_back(seg_element_fp);
                }

                offset = i * grid_w + j;
                float box[4];
                float before_dfl[dfl_len * 4];
                for (int k = 0; k < dfl_len * 4; k++)
                {
                    before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset], box_zp, box_scale);
                    offset += grid_len;
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}

static int process_fp32(rknn_output *all_input, int input_id, int grid_h, int grid_w, int height, int width, int stride, int dfl_len,
                        std::vector<float> &boxes, std::vector<float> &segments, float *proto, std::vector<float> &objProbs, std::vector<int> &classId, float threshold)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;

    // Skip if input_id is not 0, 4, 8, or 12
    if (input_id % 4 != 0)
    {
        return validCount;
    }

    if (input_id == 12)
    {
        float *input_proto = (float *)all_input[input_id].buf;
        for (int i = 0; i < PROTO_CHANNEL * PROTO_HEIGHT * PROTO_WEIGHT; i++)
        {
            proto[i] = input_proto[i];
        }
        return validCount;
    }

    float *box_tensor = (float *)all_input[input_id].buf;
    float *score_tensor = (float *)all_input[input_id + 1].buf;
    float *score_sum_tensor = (float *)all_input[input_id + 2].buf;
    float *seg_tensor = (float *)all_input[input_id + 3].buf;

    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            int offset_seg = i * grid_w + j;
            float *in_ptr_seg = seg_tensor + offset_seg;

            // for quick filtering through "score sum"
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < threshold)
                {
                    continue;
                }
            }

            float max_score = 0;
            for (int c = 0; c < OBJ_CLASS_NUM; c++)
            {
                if ((score_tensor[offset] > threshold) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > threshold)
            {

                for (int k = 0; k < PROTO_CHANNEL; k++)
                {
                    float seg_element_f32 = in_ptr_seg[(k)*grid_len];
                    segments.push_back(seg_element_f32);
                }

                offset = i * grid_w + j;
                float box[4];
                float before_dfl[dfl_len * 4];
                for (int k = 0; k < dfl_len * 4; k++)
                {
                    before_dfl[k] = box_tensor[offset];
                    offset += grid_len;
                }
                compute_dfl(before_dfl, dfl_len, box);

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(max_score);
                classId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}

int post_process(rknn_app_context_t *app_ctx, rknn_output *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results)
{

    std::vector<float> filterBoxes;
    std::vector<float> objProbs;
    std::vector<int> classId;

    std::vector<float> filterSegments;
    float proto[PROTO_CHANNEL * PROTO_HEIGHT * PROTO_WEIGHT];
    std::vector<float> filterSegments_by_nms;

    int model_in_width = app_ctx->model_width;
    int model_in_height = app_ctx->model_height;

    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;

    memset(od_results, 0, sizeof(object_detect_result_list));

    int dfl_len = app_ctx->output_attrs[0].dims[1] / 4;
    int output_per_branch = app_ctx->io_num.n_output / 3; // default 3 branch

    // process the outputs of rknn
    for (int i = 0; i < 13; i++)
    {
        grid_h = app_ctx->output_attrs[i].dims[2];
        grid_w = app_ctx->output_attrs[i].dims[3];
        stride = model_in_height / grid_h;

        if (app_ctx->is_quant)
        {
            validCount += process_i8(outputs, i, grid_h, grid_w, model_in_height, model_in_width, stride, dfl_len, filterBoxes, filterSegments, proto, objProbs,
                                     classId, conf_threshold, app_ctx);
        }
        else
        {
            validCount += process_fp32(outputs, i, grid_h, grid_w, model_in_height, model_in_width, stride, dfl_len, filterBoxes, filterSegments, proto, objProbs,
                                       classId, conf_threshold);
        }
    }

    // nms
    if (validCount <= 0)
    {
        return 0;
    }
    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i)
    {
        indexArray.push_back(i);
    }

    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));

    for (auto c : class_set)
    {
        nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
    }

    int last_count = 0;
    od_results->count = 0;

    for (int i = 0; i < validCount; ++i)
    {
        if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[i];

        for (int k = 0; k < PROTO_CHANNEL; k++)
        {
            filterSegments_by_nms.push_back(filterSegments[n * PROTO_CHANNEL + k]);
        }

        od_results->results[last_count].box.left = x1;
        od_results->results[last_count].box.top = y1;
        od_results->results[last_count].box.right = x2;
        od_results->results[last_count].box.bottom = y2;

        od_results->results[last_count].prop = obj_conf;
        od_results->results[last_count].cls_id = id;
        last_count++;
    }
    od_results->count = last_count;
    int boxes_num = od_results->count;

    float filterBoxes_by_nms[boxes_num * 4];
    int cls_id[boxes_num];
    for (int i = 0; i < boxes_num; i++)
    {
        // for crop_mask
        filterBoxes_by_nms[i * 4 + 0] = od_results->results[i].box.left;   // x1;
        filterBoxes_by_nms[i * 4 + 1] = od_results->results[i].box.top;    // y1;
        filterBoxes_by_nms[i * 4 + 2] = od_results->results[i].box.right;  // x2;
        filterBoxes_by_nms[i * 4 + 3] = od_results->results[i].box.bottom; // y2;
        cls_id[i] = od_results->results[i].cls_id;

        // get real box
        od_results->results[i].box.left = box_reverse(od_results->results[i].box.left, model_in_width, letter_box->x_pad, letter_box->scale);
        od_results->results[i].box.top = box_reverse(od_results->results[i].box.top, model_in_height, letter_box->y_pad, letter_box->scale);
        od_results->results[i].box.right = box_reverse(od_results->results[i].box.right, model_in_width, letter_box->x_pad, letter_box->scale);
        od_results->results[i].box.bottom = box_reverse(od_results->results[i].box.bottom, model_in_height, letter_box->y_pad, letter_box->scale);
    }

    TIMER timer;
#ifdef USE_FP_RESIZE
    timer.tik();
    // compute the mask through Matmul
    int ROWS_A = boxes_num;
    int COLS_A = PROTO_CHANNEL;
    int COLS_B = PROTO_HEIGHT * PROTO_WEIGHT;
    float *matmul_out = (float *)malloc(boxes_num * PROTO_HEIGHT * PROTO_WEIGHT * sizeof(float));
    matmul_by_cpu_fp(filterSegments_by_nms, proto, matmul_out, ROWS_A, COLS_A, COLS_B);
    // matmul_by_npu_fp(filterSegments_by_nms, proto, matmul_out, ROWS_A, COLS_A, COLS_B, app_ctx);
    timer.tok();
    timer.print_time("matmul_by_cpu_fp");

    timer.tik();
    // resize to (boxes_num, model_in_width, model_in_height)
    float *seg_mask = (float *)malloc(boxes_num * model_in_height * model_in_width * sizeof(float));
    resize_by_opencv_fp(matmul_out, PROTO_WEIGHT, PROTO_HEIGHT, boxes_num, seg_mask, model_in_width, model_in_height);
    timer.tok();
    timer.print_time("resize_by_opencv_fp");

    timer.tik();
    // crop mask
    uint8_t *all_mask_in_one = (uint8_t *)malloc(model_in_height * model_in_width * sizeof(uint8_t));
    memset(all_mask_in_one, 0, model_in_height * model_in_width * sizeof(uint8_t));
    crop_mask_fp(seg_mask, all_mask_in_one, filterBoxes_by_nms, boxes_num, cls_id, model_in_height, model_in_width);
    timer.tok();
    timer.print_time("crop_mask_fp");
#else
    timer.tik();
    // compute the mask through Matmul
    int ROWS_A = boxes_num;
    int COLS_A = PROTO_CHANNEL;
    int COLS_B = PROTO_HEIGHT * PROTO_WEIGHT;
    uint8_t *matmul_out = (uint8_t *)malloc(boxes_num * PROTO_HEIGHT * PROTO_WEIGHT * sizeof(uint8_t));
    matmul_by_cpu_uint8(filterSegments_by_nms, proto, matmul_out, ROWS_A, COLS_A, COLS_B);

    timer.tok();
    timer.print_time("matmul_by_cpu_uint8");

    timer.tik();
    uint8_t *seg_mask = (uint8_t *)malloc(boxes_num * model_in_height * model_in_width * sizeof(uint8_t));
    resize_by_opencv_uint8(matmul_out, PROTO_WEIGHT, PROTO_HEIGHT, boxes_num, seg_mask, model_in_width, model_in_height);
    timer.tok();
    timer.print_time("resize_by_opencv_uint8");

    timer.tik();
    // crop mask
    uint8_t *all_mask_in_one = (uint8_t *)malloc(model_in_height * model_in_width * sizeof(uint8_t));
    memset(all_mask_in_one, 0, model_in_height * model_in_width * sizeof(uint8_t));
    crop_mask_uint8(seg_mask, all_mask_in_one, filterBoxes_by_nms, boxes_num, cls_id, model_in_height, model_in_width);
    timer.tok();
    timer.print_time("crop_mask_uint8");
#endif

    timer.tik();
    // get real mask
    int cropped_height = model_in_height - letter_box->y_pad * 2;
    int cropped_width = model_in_width - letter_box->x_pad * 2;
    int ori_in_height = app_ctx->input_image_height;
    int ori_in_width = app_ctx->input_image_width;
    int y_pad = letter_box->y_pad;
    int x_pad = letter_box->x_pad;
    uint8_t *cropped_seg_mask = (uint8_t *)malloc(cropped_height * cropped_width * sizeof(uint8_t));
    uint8_t *real_seg_mask = (uint8_t *)malloc(ori_in_height * ori_in_width * sizeof(uint8_t));
    seg_reverse(all_mask_in_one, cropped_seg_mask, real_seg_mask,
                model_in_height, model_in_width, cropped_height, cropped_width, ori_in_height, ori_in_width, y_pad, x_pad);
    od_results->results_seg[0].seg_mask = real_seg_mask;
    free(all_mask_in_one);
    free(cropped_seg_mask);
    free(seg_mask);
    free(matmul_out);
    timer.tok();
    timer.print_time("seg_reverse");

    return 0;
}

int init_post_process()
{
    int ret = 0;
    ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
    if (ret < 0)
    {
        printf("Load %s failed!\n", LABEL_NALE_TXT_PATH);
        return -1;
    }
    return 0;
}

char *coco_cls_to_name(int cls_id)
{

    if (cls_id >= OBJ_CLASS_NUM)
    {
        return "null";
    }

    if (labels[cls_id])
    {
        return labels[cls_id];
    }

    return "null";
}

void deinit_post_process()
{
    for (int i = 0; i < OBJ_CLASS_NUM; i++)
    {
        {
            free(labels[i]);
            labels[i] = nullptr;
        }
    }
}
