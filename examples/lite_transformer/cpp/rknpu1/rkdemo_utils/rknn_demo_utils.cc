#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <assert.h>

// multi thread support
#include <iostream>
#include <thread>
#include <vector>

#include "rknn_demo_utils.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp)
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

static unsigned char *load_model(const char *filename, int *model_size)
{

    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}

int set_io_attrs(const char *model_name, MODEL_INFO *model_info)
{
    int ret = 0;

    // Query input/output num
    rknn_input_output_num io_num;
    ret = rknn_query(model_info->ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(rknn_input_output_num));
    if (ret != RKNN_SUCC)
    {
        printf("[%s]: query io_num failed, ret=%d!\n", model_name, ret);
        return -1;
    }
    printf("[%s]: in_num=%d, out_num=%d.\n", model_name, io_num.n_input, io_num.n_output);
    model_info->n_input = io_num.n_input;
    model_info->n_output = io_num.n_output;

    // Query input tensors attribute
    model_info->in_attrs = (rknn_tensor_attr *)malloc(sizeof(rknn_tensor_attr) * model_info->n_input);
    memset(model_info->in_attrs, 0, sizeof(rknn_tensor_attr) * model_info->n_input);
    printf("[%s]: input attributes:\n", model_name);
    for (int i = 0; i < io_num.n_input; i++)
    {
        model_info->in_attrs[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_INPUT_ATTR, (model_info->in_attrs) + i, sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("[%s]: query input attr failed, ret=%d!\n", model_name, ret);
            return -1;
        }
        dump_tensor_attr((model_info->in_attrs) + i);
    }

    // Query output tensors attribute
    model_info->out_attrs = (rknn_tensor_attr *)malloc(sizeof(rknn_tensor_attr) * model_info->n_output);
    memset(model_info->out_attrs, 0, sizeof(rknn_tensor_attr) * model_info->n_output);
    printf("[%s]: output attributes:\n", model_name);
    for (int i = 0; i < io_num.n_output; i++)
    {
        model_info->out_attrs[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_OUTPUT_ATTR, (model_info->out_attrs) + i, sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("[%s] query output attr failed, ret=%d!\n", model_name, ret);
            return -1;
        }
        dump_tensor_attr((model_info->out_attrs) + i);
    }

    return 0;
}

int init_input_buffer_all(const char *model_name, MODEL_INFO *model_info)
{
    int ret = 0;

    rknn_tensor_attr *in_attrs = model_info->in_attrs;

    // create memory for inputs
    if (model_info->use_zp)
    {
        // zero-copy
        printf("[%s] use zero-copy api, please make sure the npu driver version >= 1.7.3\n", model_name);
        model_info->in_mems = (rknn_tensor_mem **)malloc(sizeof(rknn_tensor_mem *) * model_info->n_input);
        for (int i = 0; i < model_info->n_input; i++)
        {
            model_info->in_mems[i] = rknn_create_mem(model_info->ctx, in_attrs[i].size);
            printf("[%s] init_input_buffer(zero_copy): node_index=%d, size: %d.\n", model_name, i, in_attrs[i].size);
            ret = rknn_set_io_mem(model_info->ctx, model_info->in_mems[i], &(in_attrs[i]));
            if (ret < 0)
            {
                printf("[%s] set_io_mem for input[%d] failed, ret=%d.\n", model_name, i, ret);
                return -1;
            }
        }
    }
    else
    {
        // normal api
        model_info->inputs = (rknn_input *)malloc(sizeof(rknn_input) * model_info->n_input);
        memset(model_info->inputs, 0x0, sizeof(rknn_input) * model_info->n_input);
        for (int i = 0; i < model_info->n_input; i++)
        {
            model_info->inputs[i].index = i;
            model_info->inputs[i].type = RKNN_TENSOR_FLOAT32;
            // input.fmt = RKNN_TENSOR_NHWC;
            model_info->inputs[i].size = in_attrs[i].n_elems * sizeof(float);
            // malloc buffer during inference.
            printf("[%s] init_input_buffer(normal_api): node_index=%d, size: %d.\n", model_name, i, model_info->inputs[i].size);
        }
    }

    return 0;
}

int init_output_buffer_all(const char *model_name, MODEL_INFO *model_info)
{
    int ret = 0;

    rknn_tensor_attr *out_attrs = model_info->out_attrs;

    // create memory for inputs
    if (model_info->use_zp)
    {
        // zero-copy
        printf("[%s] use zero-copy api, please make sure the npu driver version >= 1.7.3\n", model_name);
        model_info->out_mems = (rknn_tensor_mem **)malloc(sizeof(rknn_tensor_mem *) * model_info->n_output);
        for (int i = 0; i < model_info->n_output; i++)
        {
            model_info->out_mems[i] = rknn_create_mem(model_info->ctx, out_attrs[i].size);
            printf("[%s] init_output_buffer(zero_copy): node_index=%d, size: %d.\n", model_name, i, out_attrs[i].size);
            ret = rknn_set_io_mem(model_info->ctx, model_info->out_mems[i], &(out_attrs[i]));
            if (ret < 0)
            {
                printf("[%s] set_io_mem for input[%d] failed, ret=%d.\n", model_name, i, ret);
                return -1;
            }
        }
    }
    else
    {
        // normal api
        model_info->outputs = (rknn_output *)malloc(sizeof(rknn_output) * model_info->n_output);
        memset(model_info->outputs, 0x0, sizeof(rknn_output) * model_info->n_output);
        for (int i = 0; i < model_info->n_output; i++)
        {
            model_info->outputs[i].want_float = 1;
            printf("[%s] init_output_buffer(normal_api): node_index=%d, want_float: true.\n", model_name, i);
        }
    }

    return 0;
}

int rkdemo_model_init(bool is_encoder, const char *model_path, MODEL_INFO *model_info)
{
    int ret = 0;
    int model_data_size = 0;
    const char *model_name;

    if (model_path == nullptr)
    {
        printf("ERROR model path is null");
        return -1;
    }

    // define model name used to debug
    if (is_encoder)
    {
        model_name = "encoder";
    }
    else
    {
        model_name = "decoder";
    }

    // load model data
    unsigned char *model_data = load_model(model_path, &model_data_size);

    // init rknn context
    ret = rknn_init(&(model_info->ctx), (void *)model_data, model_data_size, 0);
    free(model_data);
    if (ret < 0)
    {
        printf("[%s] init RKNN model failed! ret=%d\n", model_name, ret);
        return -1;
    }

    // set input/output attributes
    ret = set_io_attrs(model_name, model_info);
    if (ret < 0)
    {
        return -1;
    }

    // init inputs/outputs buffer
    ret = init_input_buffer_all(model_name, model_info);
    if (ret < 0)
    {
        printf("[%s] init input buffer failed!", model_name);
        return -1;
    }
    ret = init_output_buffer_all(model_name, model_info);
    if (ret < 0)
    {
        printf("[%s] init output buffer failed!", model_name);
        return -1;
    }

    return ret;
}

int rkdemo_model_release(MODEL_INFO *model_info)
{
    // release inputs/outputs buffer
    if (model_info->use_zp)
    {
        for (int i = 0; i < model_info->n_input; i++)
        {
            rknn_destroy_mem(model_info->ctx, model_info->in_mems[i]);
        }
        free(model_info->in_mems);

        for (int i = 0; i < model_info->n_output; i++)
        {
            rknn_destroy_mem(model_info->ctx, model_info->out_mems[i]);
        }
        free(model_info->out_mems);
    }

    // release inputs/outputs for normal api
    if (model_info->inputs)
    {
        free(model_info->inputs);
    }
    if (model_info->outputs)
    {
        free(model_info->outputs);
    }

    // release inputs/output attributes
    if (model_info->in_attrs)
    {
        free(model_info->in_attrs);
    }
    if (model_info->out_attrs)
    {
        free(model_info->out_attrs);
    }

    // destroy rknn_context
    rknn_destroy(model_info->ctx);

    return 0;
}