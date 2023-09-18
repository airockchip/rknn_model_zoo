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

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>

// #if (TARGET_SOC==RK3399PRO)
#include <pthread.h>
// #endif


#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include "rknn_api.h"
#define NPY_SUPPORT 1
// #define RK1808 0
// #define USE_ZERO_COPY 1
#define FD_MEM 1

#if FD_MEM
#include "drm_func.h"
#endif

#if NPY_SUPPORT
#include "cnpy.h"
using namespace cnpy;
#endif

#include "type_decode.h"
using namespace std;

/*-------------------------------------------
                  Functions
-------------------------------------------*/

static void printRKNNTensor(rknn_tensor_attr *attr)
{
    printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
           attr->n_elems, attr->size, 0, attr->type, attr->qnt_type, attr->fl, attr->zp, attr->scale);
}

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

size_t getCurrentTimeMS()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static unsigned char *load_model(const char *filename, int *model_size)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == nullptr)
    {
        printf("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char *model = (unsigned char *)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if (model_len != fread(model, 1, model_len, fp))
    {
        printf("fread %s fail!\n", filename);
        free(model);
        return NULL;
    }
    *model_size = model_len;
    if (fp)
    {
        fclose(fp);
    }
    return model;
}

#if NPY_SUPPORT
static unsigned char* load_npy(const char* input_path, rknn_tensor_attr* input_attr, int* input_type, int* input_size)
{
    int req_height  = 0;
    int req_width   = 0;
    int req_channel = 0;

    printf("Loading %s\n", input_path);

    if (input_attr->n_dims == 4)
    {
        switch (input_attr->fmt){
            case RKNN_TENSOR_NHWC:
                printf("NHWC\n");
                req_height  = input_attr->dims[2];
                req_width   = input_attr->dims[1];
                req_channel = input_attr->dims[0];
                break;
            case RKNN_TENSOR_NCHW:
                printf("NCHW\n");
                req_height  = input_attr->dims[1];
                req_width   = input_attr->dims[0];
                req_channel = input_attr->dims[2];
                break;
            default:
                printf("meet unsupported layout\n");
                return NULL;
        }
    }
    else{
        printf("Input non 4-D needn't nchw/nhwc check\n");
    }

    NpyArray npy_data = npy_load(input_path);

    int         type_bytes = npy_data.word_size;
    std::string typeName   = npy_data.typeName;

    printf("npy data type:%s\n", typeName.c_str());

    if (typeName == "int8") {
        *input_type = RKNN_TENSOR_INT8;
    } else if (typeName == "uint8") {
        *input_type = RKNN_TENSOR_UINT8;
    } else if (typeName == "float16") {
        *input_type = RKNN_TENSOR_FLOAT16;
    } else if (typeName == "float32") {
        *input_type = RKNN_TENSOR_FLOAT32;
    // } else if (typeName == "8") {
    //   *input_type = RKNN_TENSOR_BOOL;
    // } else if (typeName == "int64") {
    //   *input_type = RKNN_TENSOR_INT64;
    }

    // check npy shape = NHWC
    if (input_attr->n_dims == 4)
    {
        int npy_shape[4] = {1, 1, 1, 1};

        int start = npy_data.shape.size() == 4 ? 0 : 1;
        for (size_t i = 0; i < npy_data.shape.size() && i < 4; ++i) {
            npy_shape[start + i] = npy_data.shape[i];
        }

        int height;
        int width;
        int channel;

        switch (input_attr->fmt){
            case RKNN_TENSOR_NHWC:
                height  = npy_shape[1];
                width   = npy_shape[2];
                channel = npy_shape[3];
                break;
            case RKNN_TENSOR_NCHW:
    #if USE_ZERO_COPY
                height  = npy_shape[2];
                width   = npy_shape[3];
                channel = npy_shape[1];
    #else
                height  = npy_shape[1];
                width   = npy_shape[2];
                channel = npy_shape[3];
    #endif
                break;
            default:
                printf("meet unsupported layout\n");
                return NULL;
        }

        if ((width != req_width || height != req_height || channel != req_channel)) {
            printf("npy shape match failed!, (%d, %d, %d) != (%d, %d, %d)\n", height, width, channel, req_height, req_width,
                req_channel);
            return NULL;
        }
    }

    unsigned char* data = (unsigned char*)malloc(npy_data.num_bytes());
    if (!data) {
        return NULL;
    }

    // TODO: copy
    memcpy(data, npy_data.data<unsigned char>(), npy_data.num_bytes());

    *input_size = npy_data.num_bytes();

    return data;
}

static void save_npy(const char* output_path, float* output_data, rknn_tensor_attr* output_attr)
{
  std::vector<size_t> output_shape;

  for (uint32_t i = 0; i < output_attr->n_dims; ++i) {
    output_shape.push_back(output_attr->dims[output_attr->n_dims - i - 1]); // toolkit1 is inverse
  }

  npy_save<float>(output_path, output_data, output_shape);
}
#endif

static unsigned char* load_image(const char* image_path, rknn_tensor_attr* input_attr)
{
  int req_height  = 0;
  int req_width   = 0;
  int req_channel = 0;

  switch (input_attr->fmt) {
  case RKNN_TENSOR_NHWC:
    req_height  = input_attr->dims[2];
    req_width   = input_attr->dims[1];
    req_channel = input_attr->dims[0];
    break;
  case RKNN_TENSOR_NCHW:
    req_height  = input_attr->dims[1];
    req_width   = input_attr->dims[0];
    req_channel = input_attr->dims[2];
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

static std::vector<std::string> split(const std::string& str, const std::string& pattern)
{
  std::vector<std::string> res;
  if (str == "")
    return res;
  std::string strs = str + pattern;
  size_t      pos  = strs.find(pattern);
  while (pos != strs.npos) {
    std::string temp = strs.substr(0, pos);
    res.push_back(temp);
    strs = strs.substr(pos + 1, strs.size());
    pos  = strs.find(pattern);
  }
  return res;
}

static int write_data_to_file(char* path, char* data, unsigned int size)
{
  FILE* fp;

  fp = fopen(path, "w");
  if (fp == NULL) {
    printf("open error: %s", path);
    return -1;
  }

  fwrite(data, 1, size, fp);
  fflush(fp);

  fclose(fp);
  return 0;
}


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{

    if (argc != 4) {
        printf("Usage:%s model_path input_path [loop_count] [output_dir] \n", argv[0]);
        return -1;
    }

    int            drm_fd  = -1;
    drm_context    drm_ctx;
    memset(&drm_ctx, 0, sizeof(drm_context));
    
    int            img_width   = 0;
    int            img_height  = 0;
    int            img_channel = 0;

    const char *model_path = argv[1];
    const char *input_paths = argv[2];
    std::vector<std::string> input_paths_split = split(input_paths, "#");

    int loop_count = 1;
    loop_count = atoi(argv[3]);


    rknn_context ctx;
    int ret;
    int model_len = 0;
    unsigned char *model;


    struct timeval start_time, stop_time;
    float set_time, run_time_simple, run_time_full, get_time, cvt_type_time, init_time = 0;
    float input_io_init_time, output_io_init_time = 0;
    long oldTime, newTime;


    // Load RKNN Model
    model = load_model(model_path, &model_len);
    gettimeofday(&start_time, NULL);
    ret = rknn_init(&ctx, model, model_len, 0);
    gettimeofday(&stop_time, NULL);
    init_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Info
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        printRKNNTensor(&(input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        printRKNNTensor(&(output_attrs[i]));
    }

    unsigned char* input_data[io_num.n_input];
    int            input_type[io_num.n_input];
    int            input_layout[io_num.n_input];
    int            input_size[io_num.n_input];
    for (int i = 0; i < io_num.n_input; i++) {
        input_data[i]   = NULL;
        input_type[i]   = RKNN_TENSOR_UINT8;
        input_layout[i] = RKNN_TENSOR_NHWC;
        input_size[i]   = input_attrs[i].n_elems * sizeof(uint8_t);
    }

    // Load input
    if (io_num.n_input != input_paths_split.size()) {
        return -1;
    }
    for (int i = 0; i < io_num.n_input; i++) {
      if (strstr(input_paths_split[i].c_str(), ".npy")) {
          // Load npy
#if NPY_SUPPORT
          input_data[i] = load_npy(input_paths_split[i].c_str(), &input_attrs[i], &input_type[i], &input_size[i]);
#else
          return -1;
#endif
      } else {
          // Load image
          input_data[i] = load_image(input_paths_split[i].c_str(), &input_attrs[i]);
      }
      if (!input_data[i]) {
          return -1;
      }
    }

#if USE_ZERO_COPY
    printf("use zero copy\n");
#if FD_MEM
    printf("using drm to alloc mem\n");
    drm_fd  = drm_init(&drm_ctx);

    rknn_tensor_mem* inputs_mem[io_num.n_input];
    for (int i = 0; i < io_num.n_input; i++){
        inputs_mem[i] = (rknn_tensor_mem*)malloc(sizeof(rknn_tensor_mem));
    }

    for (int i = 0; i < io_num.n_input; i++) {
        printf("alloc for inputs - %d\n", i);
        inputs_mem[i]->logical_addr = drm_buf_alloc(&drm_ctx, 
                                                    drm_fd, 
                                                    max((uint32_t)1,input_attrs[i].dims[0]), 
                                                    max((uint32_t)1,input_attrs[i].dims[1]), 
                                                    max((uint32_t)1,input_attrs[i].dims[2]) * 8, 
                                                    &(inputs_mem[i]->fd), 
                                                    (unsigned int*)&(inputs_mem[i]->handle), 
                                                    (size_t*)&(inputs_mem[i]->size));
        gettimeofday(&start_time, NULL);
        rknn_set_io_mem(ctx, inputs_mem[i], &input_attrs[i]);
        gettimeofday(&stop_time, NULL);
    }
    input_io_init_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

    rknn_tensor_mem* outputs_mem[io_num.n_output];
    for (int i = 0; i < io_num.n_output; i++){
        outputs_mem[i] = (rknn_tensor_mem*)malloc(sizeof(rknn_tensor_mem));
    }

    for (int i = 0; i < io_num.n_output; i++)
    {
        printf("alloc for outputs - %d\n", i);
        outputs_mem[i]->logical_addr = drm_buf_alloc(&drm_ctx, 
                                                    drm_fd, 
                                                    max((uint32_t)1,output_attrs[i].dims[0]), 
                                                    max((uint32_t)1,output_attrs[i].dims[1]), 
                                                    max((uint32_t)1,output_attrs[i].dims[2])*max((uint32_t)1,output_attrs[i].dims[3]) * 8, 
                                                    &(outputs_mem[i]->fd), 
                                                    (unsigned int*)&(outputs_mem[i]->handle), 
                                                    (size_t*)&(outputs_mem[i]->size));
        gettimeofday(&start_time, NULL);
        rknn_set_io_mem(ctx, outputs_mem[i], &output_attrs[i]);
        gettimeofday(&stop_time, NULL);
    }
    output_io_init_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

    printf("init drm mem success\n");
#else
    printf("using rknn_create_mem to alloc mem\n");
    rknn_tensor_mem* inputs_mem[io_num.n_input];
    rknn_tensor_mem* outputs_mem[io_num.n_output];

    for (int i = 0; i < io_num.n_input; i++) {
        inputs_mem[i] = rknn_create_mem(ctx, input_attrs[i].size);
        rknn_set_io_mem(ctx, inputs_mem[i], &input_attrs[i]);
    }

    for (int i = 0; i < io_num.n_output; i++)
    {
        outputs_mem[i] = rknn_create_mem(ctx, output_attrs[i].size);
        rknn_set_io_mem(ctx, outputs_mem[i], &output_attrs[i]);
    }
    printf("create mem success\n");
#endif

#else
    rknn_input inputs[io_num.n_input];
    memset(inputs, 0, io_num.n_input * sizeof(rknn_input));
    for (int i = 0; i < io_num.n_input; i++) {
        inputs[i].index = i;
        inputs[i].pass_through = 0;
        inputs[i].type = (rknn_tensor_type)input_type[i];
        inputs[i].fmt = (rknn_tensor_format)input_layout[i];
        inputs[i].buf = input_data[i];
        inputs[i].size = input_size[i];
    }
    // // Set input
    // ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    // if (ret < 0) {
    //   printf("rknn_input_set fail! ret=%d\n", ret);
    //   return -1;
    // }

    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        outputs[i].want_float = 1;
    }
#endif


#if USE_ZERO_COPY
    float* output_data_fp[io_num.n_output][1];
    for (int i = 0; i < io_num.n_output; i++) {
        *output_data_fp[i] = (float*)malloc(output_attrs[i].n_elems * sizeof(float));
    }

    for (int i=0;i<io_num.n_input;i++){
        memcpy(inputs_mem[i]->logical_addr, input_data[i], input_attrs[i].dims[0] * input_attrs[i].dims[1] * input_attrs[i].dims[2]);
        // rknn_set_io_mem(ctx, inputs_mem[i], &input_attrs[i]);
        // drm_buf_destroy(&drm_ctx, drm_fd, inputs_mem[i]->fd, (unsigned int)inputs_mem[i]->handle, inputs_mem[i]->logical_addr, (size_t)inputs_mem[i]->size);
    }

    printf("start run\n");
    ret = rknn_run(ctx, nullptr); // warm up
    printf("finish run\n");
    oldTime = getCurrentTimeMS();
    for (int k=0; k < loop_count; k++){
        ret = rknn_run(ctx, nullptr);
    }
    newTime = getCurrentTimeMS();
    run_time_full = (float)(newTime - oldTime);

    gettimeofday(&start_time, NULL);
    for (int i = 0; i < io_num.n_output; i++) 
    {
        if (output_attrs[i].type == RKNN_TENSOR_FLOAT16) {
            for (int j = 0; j < output_attrs[i].n_elems; j++) {
                output_data_fp[i][0][j] = decode_float16(((uint16_t*)outputs_mem[i]->logical_addr)[j]);
            }
        } else if (output_attrs[i].type == RKNN_TENSOR_UINT8) {
            for (int j = 0; j < output_attrs[i].n_elems; j++) {
                output_data_fp[i][0][j] = decode_u8(((uint8_t*)outputs_mem[i]->logical_addr)[j], output_attrs[i].scale, output_attrs[i].zp);
            }
        } else if (output_attrs[i].type == RKNN_TENSOR_INT8) {
            for (int j = 0; j < output_attrs[i].n_elems; j++) {
                output_data_fp[i][0][j] = decode_i8(((int8_t*)outputs_mem[i]->logical_addr)[j], output_attrs[i].fl);
            }
        } else if (output_attrs[i].type == RKNN_TENSOR_INT16) {
            for (int j = 0; j < output_attrs[i].n_elems; j++) {
                output_data_fp[i][0][j] = decode_i16(((int16_t*)outputs_mem[i]->logical_addr)[j], output_attrs[i].fl);
            }
        }            
    }
    gettimeofday(&stop_time, NULL);
    cvt_type_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

    set_time = set_time/loop_count;
    run_time_full = run_time_full/loop_count;
    get_time = get_time/loop_count;
    cvt_type_time = cvt_type_time;
    printf("NOTICE: New zero_copy api needn't input_sync and output_syn\n");
    printf("run_time = %f ms, cvt_type_time = %f\n", run_time_full, cvt_type_time);
#else
    for (int k=0; k < loop_count; k++){
        gettimeofday(&start_time, NULL);
        // input set
        for (int i = 0; i < io_num.n_input; i++) {
            inputs[i].buf = input_data[i];
        }
        ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
        if (ret < 0) {
            printf("rknn_input_set fail! ret=%d\n", ret);
            return -1;
        }
        gettimeofday(&stop_time, NULL);
        set_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;

        // run 
        gettimeofday(&start_time, NULL);
        ret = rknn_run(ctx, nullptr);
        if (ret != 0){
            printf("run RKNN model failed!\n");
        }

        gettimeofday(&stop_time, NULL);
        run_time_simple += (__get_us(stop_time) - __get_us(start_time)) / 1000;
        printf("run_time = %f ms\n",(__get_us(stop_time) - __get_us(start_time)) / 1000);

        // output get
        gettimeofday(&start_time, NULL);
        rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
        rknn_outputs_release(ctx, io_num.n_output, outputs);
        gettimeofday(&stop_time, NULL);
        get_time += (__get_us(stop_time) - __get_us(start_time)) / 1000;
    }

#if RK1808
    oldTime = getCurrentTimeMS();
    for (int k=0; k < loop_count; k++){
        ret = rknn_run(ctx, nullptr);
    }
    newTime = getCurrentTimeMS();
    run_time_simple = (float)(newTime - oldTime);
    printf("For RK1808, get_time may not compute correctly, please refer to total time\n");
#endif 

    oldTime = getCurrentTimeMS();
    for (int k=0; k < loop_count; k++){
        rknn_inputs_set(ctx, io_num.n_input, inputs);
        ret = rknn_run(ctx, nullptr);
        rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
    }
    newTime = getCurrentTimeMS();
    run_time_full = (float)(newTime - oldTime);


    set_time = set_time/loop_count;
    run_time_simple = run_time_simple/loop_count;
    run_time_full = run_time_full/loop_count;
    get_time = get_time/loop_count;
    printf("set_time = %f ms, run_time = %f ms, get_time = %f ms\n", set_time, run_time_simple, get_time);
    printf("total time = %f ms\n", run_time_full);
#endif
    
    // record time
    FILE *output_file = NULL;
    output_file = fopen("./capi_record.txt", "w+");
    fprintf(output_file, "model name: %s\n", model_path);
    fprintf(output_file, "output number: %d\n", io_num.n_output);
    fprintf(output_file, "loop_count: %d\n", loop_count);
# if USE_ZERO_COPY
    fprintf(output_file, "infer type: zero_copy\n");
    // fprintf(output_file, "input_syn_time: %f ms\nrun_time: %f ms\noutput_syn_time: %f ms\ncvt_type_time: %f ms\ntotal_time: %f ms\n", set_time, run_time_simple, get_time, cvt_type_time, run_time_full);
    fprintf(output_file, "model_init: %f ms\nrun: %f ms\ncpu_dequantize: %f ms\ntotal_time: %f ms\ninput_io_init: %f ms\noutput_io_init: %f ms\n", init_time, run_time_full, cvt_type_time, run_time_full, input_io_init_time, output_io_init_time);
# else
    fprintf(output_file, "infer type: normal\n");
    fprintf(output_file, "model_init: %f ms\ninput_set: %f ms\nrun: %f ms\noutput_get: %f ms\ntotal_time: %f ms\n", init_time, set_time, run_time_simple, get_time, run_time_full);
# endif

    // save npy
#if NPY_SUPPORT
#if USE_ZERO_COPY
    for (int i = 0; i < io_num.n_output; i++) {
        char output_file_name[100];
        sprintf(output_file_name, "./output_%d.npy", i);
        save_npy(output_file_name, (float*)output_data_fp[i][0], &output_attrs[i]);
    }
#else
    for (int i = 0; i < io_num.n_output; i++) {
        char output_file_name[100];
        sprintf(output_file_name, "./output_%d.npy", i);
        save_npy(output_file_name, (float*)outputs[i].buf, &output_attrs[i]);
    }
#endif
#endif

#if USE_ZERO_COPY
#if FD_MEM
    printf("! drm in deinit\n");
    for (int i = 0; i < io_num.n_input; i++) {
        printf("inputs_mem[i]->fd : %d\n", inputs_mem[i]->fd);
        printf("inputs_mem[i]->handle : %d\n", inputs_mem[i]->handle);
        printf("inputs_mem[i]->size : %d\n", inputs_mem[i]->size);
        printf("inputs_mem[i]->logical_addr : 0X%p\n", inputs_mem[i]->logical_addr);
        drm_buf_destroy(&drm_ctx, drm_fd, inputs_mem[i]->fd, (unsigned int)inputs_mem[i]->handle, inputs_mem[i]->logical_addr, (size_t)inputs_mem[i]->size);
        // free(inputs_mem[i]);
    }
    printf("! drm out deinit\n");
    for (int i = 0; i < io_num.n_output; i++) {
        printf("outputs_mem[i]->fd : %d\n", outputs_mem[i]->fd);
        printf("outputs_mem[i]->handle : %d\n", outputs_mem[i]->handle);
        printf("outputs_mem[i]->size : %d\n", outputs_mem[i]->size);
        printf("outputs_mem[i]->logical_addr : 0X%p\n", outputs_mem[i]->logical_addr);
        drm_buf_destroy(&drm_ctx, drm_fd, outputs_mem[i]->fd, (unsigned int)outputs_mem[i]->handle, outputs_mem[i]->logical_addr, (size_t)outputs_mem[i]->size);
        // free(outputs_mem[i]);
    }
    drm_deinit(&drm_ctx, drm_fd);
    printf("! drm deinit\n");
#else
    for (int i = 0; i < io_num.n_input; i++)
    {
        rknn_destroy_mem(ctx, inputs_mem[i]);
    }

    for (int i = 0; i < io_num.n_output; i++)
    {
        rknn_destroy_mem(ctx, outputs_mem[i]);
    }   
    printf("! mem destroy\n");
#endif
    for (int i = 0; i < io_num.n_output; i++) {
        free(output_data_fp[i][0]);
    }
#endif

    // rknn_perf_detail perf_detail;
    // ret = rknn_query(ctx, RKNN_QUERY_PERF_DETAIL, &perf_detail,
    // sizeof(rknn_perf_detail));
    // printf("%s", perf_detail.perf_data);

    // Release
    if (ctx >= 0)
    {
        rknn_destroy(ctx);
    }
    if (model)
    {
        free(model);
    }

    // free data
    for (int i = 0; i < io_num.n_input; i++)
    {
        free(input_data[i]);
    }

    return 0;
}
