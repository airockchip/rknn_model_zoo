/****************************************************************************
 *
 *    Copyright (c) 2017 - 2018 by Rockchip Corp.  All rights reserved.
 *
 *    The material in this file is confidential and contains trade secrets
 *    of Rockchip Corporation. This is proprietary information owned by
 *    Rockchip Corporation. No part of this work may be disclosed,
 *    reproduced, copied, transmitted, or used in any way for any purpose,
 *    without the express written permission of Rockchip Corporation.
 *
 *****************************************************************************/

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include "rknn_api.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#define NPY_SUPPORT 1

#if NPY_SUPPORT
#  include "cnpy.h"
#endif

#define MAX_INPUT_OUTPUT_NUM 10

/*-------------------------------------------
                  Functions
-------------------------------------------*/
static inline int64_t getCurrentTimeUs()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

static int rknn_GetTopN(float* pfProb, float* pfMaxProb, uint32_t* pMaxClass, uint32_t outputCount, uint32_t topNum)
{
  uint32_t i, j;
  uint32_t top_count = outputCount > topNum ? topNum : outputCount;

  for (i = 0; i < topNum; ++i) {
    pfMaxProb[i] = -FLT_MAX;
    pMaxClass[i] = -1;
  }

  for (j = 0; j < top_count; j++) {
    for (i = 0; i < outputCount; i++) {
      if ((i == *(pMaxClass + 0)) || (i == *(pMaxClass + 1)) || (i == *(pMaxClass + 2)) || (i == *(pMaxClass + 3)) ||
          (i == *(pMaxClass + 4))) {
        continue;
      }

      float prob = pfProb[i];
      if (prob > *(pfMaxProb + j)) {
        *(pfMaxProb + j) = prob;
        *(pMaxClass + j) = i;
      }
    }
  }

  return 1;
}

static int rknn_GetTopN_int8(int8_t* pProb, float scale, int zp, float* pfMaxProb, uint32_t* pMaxClass,
                             uint32_t outputCount, uint32_t topNum)
{
  uint32_t i, j;
  uint32_t top_count = outputCount > topNum ? topNum : outputCount;

  for (i = 0; i < topNum; ++i) {
    pfMaxProb[i] = -FLT_MAX;
    pMaxClass[i] = -1;
  }

  for (j = 0; j < top_count; j++) {
    for (i = 0; i < outputCount; i++) {
      if ((i == *(pMaxClass + 0)) || (i == *(pMaxClass + 1)) || (i == *(pMaxClass + 2)) || (i == *(pMaxClass + 3)) ||
          (i == *(pMaxClass + 4))) {
        continue;
      }

      float prob = (pProb[i] - zp) * scale;
      if (prob > *(pfMaxProb + j)) {
        *(pfMaxProb + j) = prob;
        *(pMaxClass + j) = i;
      }
    }
  }

  return 1;
}

static void dump_tensor_attr(rknn_tensor_attr* attr)
{
  char dims[128] = {0};
  for (int i = 0; i < attr->n_dims; ++i) {
    int idx = strlen(dims);
    sprintf(&dims[idx], "%d%s", attr->dims[i], (i == attr->n_dims - 1) ? "" : ", ");
  }
  printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, w_stride = %d, size_with_stride = %d, "
         "fmt=%s, type=%s, qnt_type=%s, "
         "zp=%d, scale=%f\n",
         attr->index, attr->name, attr->n_dims, dims, attr->n_elems, attr->size, attr->w_stride, attr->size_with_stride,
         get_format_string(attr->fmt), get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp,
         attr->scale);
}

static void* load_file(const char* file_path, size_t* file_size)
{
  FILE* fp = fopen(file_path, "r");
  if (fp == NULL) {
    printf("failed to open file: %s\n", file_path);
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  size_t size = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);

  void* file_data = malloc(size);
  if (file_data == NULL) {
    fclose(fp);
    printf("failed allocate file size: %zu\n", size);
    return NULL;
  }

  if (fread(file_data, 1, size, fp) != size) {
    fclose(fp);
    free(file_data);
    printf("failed to read file data!\n");
    return NULL;
  }

  fclose(fp);

  *file_size = size;

  return file_data;
}

static unsigned char* load_image(const char* image_path, rknn_tensor_attr* input_attr)
{
  int req_height  = 0;
  int req_width   = 0;
  int req_channel = 0;

  switch (input_attr->fmt) {
  case RKNN_TENSOR_NHWC:
    req_height  = input_attr->dims[1];
    req_width   = input_attr->dims[2];
    req_channel = input_attr->dims[3];
    break;
  case RKNN_TENSOR_NCHW:
    req_height  = input_attr->dims[2];
    req_width   = input_attr->dims[3];
    req_channel = input_attr->dims[1];
    break;
  default:
    printf("meet unsupported layout\n");
    return NULL;
  }

  int height  = 0;
  int width   = 0;
  int channel = 0;

  unsigned char* image_data = stbi_load(image_path, &width, &height, &channel, req_channel);
  if (image_data == NULL) {
    printf("load image failed!\n");
    return NULL;
  }

  if (width != req_width || height != req_height) {
    unsigned char* image_resized = (unsigned char*)STBI_MALLOC(req_width * req_height * req_channel);
    if (!image_resized) {
      printf("malloc image failed!\n");
      STBI_FREE(image_data);
      return NULL;
    }
    if (stbir_resize_uint8(image_data, width, height, 0, image_resized, req_width, req_height, 0, channel) != 1) {
      printf("resize image failed!\n");
      STBI_FREE(image_data);
      return NULL;
    }
    STBI_FREE(image_data);
    image_data = image_resized;
  }

  return image_data;
}

#if NPY_SUPPORT
static unsigned char* load_npy(const char* input_path, rknn_tensor_attr* input_attr, int* input_type, int* input_size)
{
  int req_height  = 0;
  int req_width   = 0;
  int req_channel = 0;

  printf("Loading %s\n", input_path);

  switch (input_attr->fmt) {
  case RKNN_TENSOR_NHWC:
    req_height  = input_attr->dims[1];
    req_width   = input_attr->dims[2];
    req_channel = input_attr->dims[3];
    break;
  case RKNN_TENSOR_NCHW:
    req_height  = input_attr->dims[2];
    req_width   = input_attr->dims[3];
    req_channel = input_attr->dims[1];
    break;
  case RKNN_TENSOR_UNDEFINED:
    break;
  default:
    printf("meet unsupported layout\n");
    return NULL;
  }

  cnpy_array npy_data;

  bool writable = false;
  if (cnpy_open(input_path, writable, &npy_data) != CNPY_SUCCESS) {
    printf("Unable to load file %s\n", input_path);
    return NULL;
  }

  int        data_bytes = npy_data.raw_data_size - npy_data.data_begin;
  cnpy_dtype dtype      = npy_data.dtype;

  if (dtype == CNPY_I8) {
    *input_type = RKNN_TENSOR_INT8;
  } else if (dtype == CNPY_U8) {
    *input_type = RKNN_TENSOR_UINT8;
  } else if (dtype == CNPY_F4) {
    *input_type = RKNN_TENSOR_FLOAT32;
  }

  // npy shape = NHWC
  int npy_shape[4] = {1, 1, 1, 1};

  int start = npy_data.n_dim == 4 ? 0 : 1;
  for (size_t i = 0; i < npy_data.n_dim && i < 4; ++i) {
    npy_shape[start + i] = npy_data.dims[i];
  }

  int height  = npy_shape[1];
  int width   = npy_shape[2];
  int channel = npy_shape[3];

  if ((input_attr->fmt != RKNN_TENSOR_UNDEFINED) &&
      (width != req_width || height != req_height || channel != req_channel)) {
    printf("npy shape match failed!, (%d, %d, %d) != (%d, %d, %d)\n", height, width, channel, req_height, req_width,
           req_channel);
    return NULL;
  }

  unsigned char* data = (unsigned char*)malloc(data_bytes);
  if (!data) {
    return NULL;
  }

  // TODO: copy
  memcpy(data, npy_data.raw_data + npy_data.data_begin, data_bytes);

  *input_size = data_bytes;

  return data;
}

static int save_npy(const char* output_path, float* output_data, rknn_tensor_attr* output_attr)
{
  int size = 1;

  for (uint32_t i = 0; i < output_attr->n_dims; ++i) {
    size *= output_attr->dims[i];
  }

  cnpy_array      npy_data;
  cnpy_byte_order byte_order = CNPY_LE;      /* little endian */
  cnpy_dtype      dtype      = CNPY_F4;      /* float */
  cnpy_flat_order order      = CNPY_C_ORDER; /* Fortran (row major) order */

  if (cnpy_create(output_path, byte_order, dtype, order, output_attr->n_dims, (const size_t*)output_attr->dims,
                  &npy_data) != CNPY_SUCCESS) {
    cnpy_perror("Unable to create file: ");
    return -1;
  }

  memcpy(npy_data.raw_data + npy_data.data_begin, (uint8_t*)output_data, sizeof(float) * size);

  /* optional: */
  if (cnpy_close(&npy_data) != CNPY_SUCCESS) {
    cnpy_perror("Unable to close file: ");
    return -1;
  }
  return 0;
}
#endif

int NHWC_To_NCHW_Witch_CAligin(float* src, float* dst, int batchs, int height, int width, int aligin_channel,
                               int real_channel)
{
  for (int b = 0; b < batchs; b++) {
    const float* input_src = src + b * aligin_channel * height * width;
    float*       input_dst = dst + b * real_channel * height * width;
    for (int c = 0; c < real_channel; c++) {
      for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
          input_dst[c * height * width + h * width + w] =
            input_src[h * width * aligin_channel + w * aligin_channel + c];
        }
      }
    }
  }

  return 0;
}

/*-------------------------------------------
                  Main Functions
-------------------------------------------*/
int main(int argc, char* argv[])
{
  if (argc < 3) {
    printf("Usage:%s model_path input_path [loop_count] [output_dir]\n", argv[0]);
    return -1;
  }

  char* model_path = argv[1];
  char* input_path = argv[2];

  double model_init_time, input_io_init_time, output_io_init_time, inference_time = 0;
  int64_t start_us, elapse_us = 0;

  int   input_number = 0;
  char* input_paths_split[MAX_INPUT_OUTPUT_NUM];
  for (int i = 0; i < MAX_INPUT_OUTPUT_NUM; i++) {
    input_paths_split[i] = malloc(100);
  }
  char* token = strtok(input_path, "#");
  while (token != NULL) {
    printf(" %s\n", token); // printing each token
    memcpy(input_paths_split[input_number], token, strlen(token));
    input_number++;
    token = strtok(NULL, "#");
  }
  printf("input number - %d\n", input_number);

  int loop_count = 1;
  if (argc > 3) {
    loop_count = atoi(argv[3]);
  }

  char* output_dir = NULL;
  if (argc > 4) {
    output_dir = argv[4];
  }

  rknn_context ctx = 0;

  start_us  = getCurrentTimeUs();
  // Load RKNN Model
#if 1
  // Init rknn from model path
  int ret = rknn_init(&ctx, model_path, 0, 0, NULL);
#else
  // Init rknn from model data
  size_t model_size;
  void*  model_data = load_file(model_path, &model_size);
  if (model_data == NULL) {
    return -1;
  }
  int ret = rknn_init(&ctx, model_data, model_size, 0, NULL);
  free(model_data);
#endif
  if (ret < 0) {
    printf("rknn_init fail! ret=%d\n", ret);
    return -1;
  }
  elapse_us = getCurrentTimeUs();
  model_init_time = (start_us - elapse_us)/1000.f;

  // Get sdk and driver version
  rknn_sdk_version sdk_ver;
  ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &sdk_ver, sizeof(sdk_ver));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    goto ctx_destroy;
  }
  printf("rknn_api/rknnrt version: %s, driver version: %s\n", sdk_ver.api_version, sdk_ver.drv_version);

  // Get weight and internal mem size, dma used size
  rknn_mem_size mem_size;
  ret = rknn_query(ctx, RKNN_QUERY_MEM_SIZE, &mem_size, sizeof(mem_size));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    return -1;
  }
  printf("total weight size: %d, total internal size: %d\n", mem_size.total_weight_size, mem_size.total_internal_size);
  printf("total dma used size: %zu\n", (size_t)mem_size.total_dma_allocated_size);

  // Get Model Input Output Info
  rknn_input_output_num io_num;
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    goto ctx_destroy;
  }
  printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

  printf("input tensors:\n");
  rknn_tensor_attr input_attrs[MAX_INPUT_OUTPUT_NUM];
  memset(input_attrs, 0, io_num.n_input * sizeof(rknn_tensor_attr));
  for (uint32_t i = 0; i < io_num.n_input; i++) {
    input_attrs[i].index = i;
    // query info
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
    if (ret < 0) {
      printf("rknn_init error! ret=%d\n", ret);
      goto ctx_destroy;
    }
    dump_tensor_attr(&input_attrs[i]);
  }

  printf("output tensors:\n");
  rknn_tensor_attr output_attrs[MAX_INPUT_OUTPUT_NUM];
  memset(output_attrs, 0, io_num.n_output * sizeof(rknn_tensor_attr));
  for (uint32_t i = 0; i < io_num.n_output; i++) {
    output_attrs[i].index = i;
    // query info
    ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      printf("rknn_query fail! ret=%d\n", ret);
      goto ctx_destroy;
    }
    dump_tensor_attr(&output_attrs[i]);
  }

  // Get custom string
  rknn_custom_string custom_string;
  ret = rknn_query(ctx, RKNN_QUERY_CUSTOM_STRING, &custom_string, sizeof(custom_string));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    goto ctx_destroy;
  }
  printf("custom string: %s\n", custom_string.string);

  // Get model memory
  rknn_tensor_mem mem_info;
  memset(&mem_info, 0, sizeof(mem_info));
  ret = rknn_query(ctx, RKNN_QUERY_DEVICE_MEM_INFO, &mem_info, sizeof(mem_info));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    goto ctx_destroy;
  }
  printf("model memory virt_addr=%p, phys_addr=%lu, fd=%d, size=%u\n", mem_info.virt_addr, mem_info.phys_addr,
         mem_info.fd, mem_info.size);

  unsigned char* input_data[MAX_INPUT_OUTPUT_NUM];
  int            input_type[MAX_INPUT_OUTPUT_NUM];
  int            input_layout[MAX_INPUT_OUTPUT_NUM];
  int            input_size[MAX_INPUT_OUTPUT_NUM];
  for (int i = 0; i < io_num.n_input; i++) {
    input_data[i]   = NULL;
    input_type[i]   = RKNN_TENSOR_UINT8;
    input_layout[i] = RKNN_TENSOR_NHWC;
    input_size[i]   = input_attrs[i].n_elems * sizeof(uint8_t);
  }

  // Load image
    // Load input
  if (io_num.n_input != input_number) {
    return -1;
  }

  for (int i = 0; i < io_num.n_input; i++) {
    if (strstr(input_paths_split[i], ".npy")) {
// Load npy
#if NPY_SUPPORT
      input_data[i] = load_npy(input_paths_split[i], &input_attrs[i], &input_type[i], &input_size[i]);
#else
      return -1;
#endif
    } else {
      // Load image
      input_data[i] = load_image(input_paths_split[i], &input_attrs[i]);
    }

    if (!input_data[i]) {
      return -1;
    }
  }

  // Create input tensor memory
  rknn_tensor_mem* input_mems[MAX_INPUT_OUTPUT_NUM];
  for (uint32_t i = 0; i < io_num.n_input; ++i) {
    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs[i].type = input_type[i];
    // default fmt is NHWC, npu only support NHWC in zero copy mode
    input_attrs[i].fmt = input_layout[i];

    input_mems[i] = rknn_create_mem(ctx, input_attrs[i].size_with_stride);

    // Copy input data to input tensor memory
    int width  = input_attrs[i].dims[2];
    int stride = input_attrs[i].w_stride;

    if (width == stride) {
      memcpy(input_mems[i]->virt_addr, input_data[i], width * input_attrs[i].dims[1] * input_attrs[i].dims[3]);
    } else {
      int height  = input_attrs[i].dims[1];
      int channel = input_attrs[i].dims[3];
      // copy from src to dst with stride
      uint8_t* src_ptr = input_data[i];
      uint8_t* dst_ptr = (uint8_t*)input_mems[i]->virt_addr;
      // width-channel elements
      int src_wc_elems = width * channel;
      int dst_wc_elems = stride * channel;
      for (int h = 0; h < height; ++h) {
        memcpy(dst_ptr, src_ptr, src_wc_elems);
        src_ptr += src_wc_elems;
        dst_ptr += dst_wc_elems;
      }
    }
  }

  // Create output tensor memory
  rknn_tensor_mem* output_mems[MAX_INPUT_OUTPUT_NUM];
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    output_mems[i] = rknn_create_mem(ctx, output_attrs[i].n_elems * sizeof(float));
  }

  start_us = getCurrentTimeUs();
  // Set input tensor memory
  for (uint32_t i = 0; i < io_num.n_input; ++i) {
    ret = rknn_set_io_mem(ctx, input_mems[i], &input_attrs[i]);
    if (ret < 0) {
      printf("rknn_set_io_mem fail! ret=%d\n", ret);
      goto free_mem;
    }
  }
  elapse_us = getCurrentTimeUs();
  input_io_init_time = (start_us - elapse_us)/1000.f;

  start_us = getCurrentTimeUs();
  // Set output tensor memory
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    // set output memory and attribute
    output_attrs[i].type = RKNN_TENSOR_FLOAT32;
    ret                  = rknn_set_io_mem(ctx, output_mems[i], &output_attrs[i]);
    if (ret < 0) {
      printf("rknn_set_io_mem fail! ret=%d\n", ret);
      goto free_mem;
    }
  }
  elapse_us = getCurrentTimeUs();
  output_io_init_time = (start_us - elapse_us)/1000.f;

  // Run
  printf("Begin perf ...\n");
  for (int i = 0; i < loop_count; ++i) {
    start_us  = getCurrentTimeUs();
    ret               = rknn_run(ctx, NULL);
    elapse_us = getCurrentTimeUs() - start_us;
    if (ret < 0) {
      printf("rknn run error %d\n", ret);
      goto free_mem;
    }
    inference_time += elapse_us/1000.f;
    printf("%4d: Elapse Time = %.2fms, FPS = %.2f\n", i, elapse_us / 1000.f, 1000.f * 1000.f / elapse_us);
  }
  inference_time = inference_time/ loop_count;

  printf("output origin tensors:\n");
  rknn_tensor_attr orig_output_attrs[MAX_INPUT_OUTPUT_NUM];
  memset(orig_output_attrs, 0, io_num.n_output * sizeof(rknn_tensor_attr));
  for (uint32_t i = 0; i < io_num.n_output; i++) {
    orig_output_attrs[i].index = i;
    // query info
    ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(orig_output_attrs[i]), sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      printf("rknn_query fail! ret=%d\n", ret);
      goto free_mem;
    }
    dump_tensor_attr(&orig_output_attrs[i]);
  }

  float* output_mems_nchw[MAX_INPUT_OUTPUT_NUM];
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    output_mems_nchw[i] = (float*)malloc(orig_output_attrs[i].n_elems * sizeof(float));
  }

  for (uint32_t i = 0; i < io_num.n_output; i++) {
    NHWC_To_NCHW_Witch_CAligin((float*)output_mems[i]->virt_addr, (float*)output_mems_nchw[i], output_attrs[i].dims[0],
                               output_attrs[i].dims[1], output_attrs[i].dims[2], output_attrs[i].dims[3],
                               orig_output_attrs[i].dims[1]);
  }

  // save output
  for (uint32_t i = 0; i < io_num.n_output; i++) {
    char output_path[PATH_MAX];
    sprintf(output_path, "./output_%d.npy", i);
    save_npy(output_path, (float*)output_mems_nchw[i], &orig_output_attrs[i]);
  }

  // record time
  FILE *output_file = NULL;
  output_file = fopen("./capi_record.txt", "w+");
  fprintf(output_file, "model name: %s\n", model_path);
  fprintf(output_file, "output number: %d\n", io_num.n_output);
  fprintf(output_file, "loop_count: %d\n", loop_count);
  fprintf(output_file, "infer type: zero_copy\n");
  fprintf(output_file, "model_init: %f ms\ninput_io_init: %f ms\noutput_io_init: %f ms\nrun: %f ms\n", 
          model_init_time,
          input_io_init_time,
          output_io_init_time,
          inference_time);

  // Destroy rknn memory
  for (uint32_t i = 0; i < io_num.n_input; ++i) {
    rknn_destroy_mem(ctx, input_mems[i]);
  }
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    rknn_destroy_mem(ctx, output_mems[i]);
    free(output_mems_nchw[i]);
  }

  // destroy
  rknn_destroy(ctx);

  for (uint32_t i = 0; i < input_number; ++i) {
    free(input_data[i]);
  }

  for (uint32_t i = 0; i < MAX_INPUT_OUTPUT_NUM; ++i) {
    free(input_paths_split[i]);
  }

  return 0;

ctx_destroy:
  rknn_destroy(ctx);

free_mem:
  for (uint32_t j = 0; j < io_num.n_input; ++j) {
    rknn_destroy_mem(ctx, input_mems[j]);
  }
  for (uint32_t j = 0; j < io_num.n_output; ++j) {
    rknn_destroy_mem(ctx, output_mems[j]);
  }
  rknn_destroy(ctx);
  for (uint32_t j = 0; j < input_number; ++j) {
    free(input_data[j]);
  }
  for (uint32_t i = 0; i < MAX_INPUT_OUTPUT_NUM; ++i) {
    free(input_paths_split[i]);
  }
  return -1;
}
