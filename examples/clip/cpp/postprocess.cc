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
#include <arm_neon.h>

#include "clip.h"

typedef struct {
    float value;
    int index;
} element_t;

static void swap(element_t* a, element_t* b) {
    element_t temp = *a;
    *a = *b;
    *b = temp;
}

static int partition(element_t arr[], int low, int high) {
    float pivot = arr[high].value;
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (arr[j].value >= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }

    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

static void quick_sort(element_t arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}


static void softmax(float* arr, int size)
{
    // Find the maximum value in the array
    float max_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }

    // Subtract the maximum value from each element to avoid overflow
    for (int i = 0; i < size; i++) {
        arr[i] -= max_val;
    }

    // Compute the exponentials and sum
    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        arr[i] = expf(arr[i]);
        sum += arr[i];
    }

    // Normalize the array by dividing each element by the sum
    for (int i = 0; i < size; i++) {
        arr[i] /= sum;
    }
}

static void element_multiply(float* arr, int size, const float scale)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] *= scale;
    }
}

static void matmul_by_cpu(float* A, float* B, float* out, int A_rows, int A_B_cols, int B_rows)
{
    // A: [M, N]    B: [K, N]    out: [M, K]
    float temp;
    for (int i = 0; i < A_rows; i++)
    {
        for (int j = 0; j < B_rows; j++)
        {
            temp = 0;
            for (int k = 0; k < A_B_cols; k++)
            {
                temp += A[i * A_B_cols + k] * B[j * A_B_cols + k];
            }
            out[i * B_rows + j] = temp;
        }
    }
}

static void matmul_by_cpu_optimize(float* A, float* B, float* out, int A_rows, int A_B_cols, int B_rows)
{
    // A: [M, N]    B: [K, N]    out: [M, K]
    for (int i = 0; i < A_rows; i++) {
        int k = 0;
        for (; k < (A_B_cols & ~3); k += 4) {
            float a0 = A[i * A_B_cols + k];
            float a1 = A[i * A_B_cols + k + 1];
            float a2 = A[i * A_B_cols + k + 2];
            float a3 = A[i * A_B_cols + k + 3];

            float32x4_t vA0 = vdupq_n_f32(a0);
            float32x4_t vA1 = vdupq_n_f32(a1);
            float32x4_t vA2 = vdupq_n_f32(a2);
            float32x4_t vA3 = vdupq_n_f32(a3);

            int j = 0;

            for (; j < (B_rows & ~3); j += 4) {
                float32x4_t vB0 = vld1q_f32(B + j * A_B_cols + k);
                float32x4_t vB1 = vld1q_f32(B + (j + 1) * A_B_cols + k);
                float32x4_t vB2 = vld1q_f32(B + (j + 2) * A_B_cols + k);
                float32x4_t vB3 = vld1q_f32(B + (j + 3) * A_B_cols + k);

                float32x4_t vC = vld1q_f32(out + i * B_rows + j);
                vC = vfmaq_f32(vC, vA0, vB0);
                vC = vfmaq_f32(vC, vA1, vB1);
                vC = vfmaq_f32(vC, vA2, vB2);
                vC = vfmaq_f32(vC, vA3, vB3);
                vst1q_f32(out + i * B_rows + j, vC);
            }

            for (; j < B_rows; j++) {
                out[i * B_rows + j] += a0 * B[j * A_B_cols + k];
                out[i * B_rows + j] += a1 * B[j * A_B_cols + k + 1];
                out[i * B_rows + j] += a2 * B[j * A_B_cols + k + 2];
                out[i * B_rows + j] += a3 * B[j * A_B_cols + k + 3];
            }
        }
        for (; k < A_B_cols; k++) {
            float a0 = A[i * A_B_cols + k];

            for (int j = 0; j < B_rows; j++) {
                out[i * B_rows + j] += a0 * B[j * A_B_cols + k];
            }
        }
    }
}

static void get_result_with_index(float* arr, int size, int text_num, clip_res* res)
{
    // Create an array of elements, saving values ​​and index numbers
    element_t* elements = (element_t*)malloc(size * sizeof(element_t));
    for (int i = 0; i < size; i++) {
        elements[i].value = arr[i];
        elements[i].index = i;
    }

    // Quick sort an array of elements
    quick_sort(elements, 0, size - 1);

    // Get the maximum values ​​and their index numbers
    res->img_index = elements[0].index / text_num;
    res->text_index = elements[0].index % text_num;
    res->score = elements[0].value;

    free(elements);
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


int post_process(rknn_app_context_t* app_ctx, float* img_output, float* text_output, clip_res* out_res)
{
    int out_size = app_ctx->input_img_num * app_ctx->input_text_num;
    float* matmul_out = (float*)malloc(out_size * sizeof(float));
    float logit_scale = 4.605170249938965;

    matmul_by_cpu_optimize(img_output, text_output, matmul_out, app_ctx->input_img_num, app_ctx->text.output_attrs[0].dims[1], app_ctx->input_text_num);

    element_multiply(matmul_out, out_size, expf(logit_scale));

    softmax(matmul_out, out_size);

    get_result_with_index(matmul_out, out_size, app_ctx->input_text_num, out_res);

    if (matmul_out != NULL)
    {
        free(matmul_out);
    }

    return 0;
}