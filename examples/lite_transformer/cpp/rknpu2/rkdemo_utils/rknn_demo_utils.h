/*
 #
 #  File            : rknn_demo_utils.h
 #                    ( C++ header file )
 #
 #  Description     : Due to the initialization of rknn_input and rknn_output is complex,
 #                    especially when the modols has multiple inputs and outputs. I create
 #                    this tools to control the initialization and release of rknn_input/output.
 #                    
 #                    Remember this tools is only for rknn_input/output memory management.
 #                    Not including model invoke.
 #
*/

#ifndef _RKNN_DEMO_UTILS_H
#define _RKNN_DEMO_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <assert.h>

#include <rknn_api.h>

#define _DEMO_NOT_EXITS_FMT -1


typedef enum{
    NORMAL_API = 0,
    ZERO_COPY_API,
} API_TYPE;

typedef struct _RKDEMO_INPUT_PARAM
{
    /*
        RKNN_INPUT has follow param:
        index, buf, size, pass_through, fmt, type

        Here we keep:
            pass_through,
            'fmt' as 'layout_fmt',
            'type' as 'dtype'
        
        And add:
            api_type to record normal_api/ zero_copy_api
            enable to assign if this param was used
            _already_init to record if this param was already init
    */
    uint8_t pass_through;
    rknn_tensor_format layout_fmt;
    rknn_tensor_type dtype;
    
    API_TYPE api_type;
    bool enable = false;
    bool _already_init = false;
} RKDEMO_INPUT_PARAM;

typedef struct _RKDEMO_OUTPUT_PARAM
{
    uint8_t want_float;

    API_TYPE api_type;
    bool enable = false;
    bool _already_init = false;
} RKDEMO_OUTPUT_PARAM;

typedef struct _MODEL_INFO{
    const char* m_path = nullptr;
    rknn_context ctx;
    bool is_dyn_shape = false;

    int n_input;
    rknn_tensor_attr* in_attr = nullptr;
    rknn_tensor_attr* in_attr_native = nullptr;
    rknn_input *inputs;
    rknn_tensor_mem **input_mem;
    RKDEMO_INPUT_PARAM *rkdmo_input_param;
    
    // bool inputs_alreay_init = false;

    int n_output;
    rknn_tensor_attr* out_attr = nullptr;
    rknn_tensor_attr* out_attr_native = nullptr;
    rknn_output *outputs;
    rknn_tensor_mem **output_mem;
    RKDEMO_OUTPUT_PARAM *rkdmo_output_param;

    // bool outputs_alreay_init = false;
    bool verbose_log = false;
    int diff_input_idx = -1;

    // memory could be set ousider
    int init_flag = 0;

    rknn_input_range* dyn_range;
    rknn_mem_size mem_size;
    rknn_tensor_mem* internal_mem_outside = nullptr;
    rknn_tensor_mem* internal_mem_max = nullptr;

} MODEL_INFO;

// static void dump_tensor_attr(rknn_tensor_attr *attr);
void dump_tensor_attr(rknn_tensor_attr *attr);
int rkdemo_get_type_size(rknn_tensor_type type);

int rkdemo_init(MODEL_INFO* model_info);
int rkdemo_init_share_weight(MODEL_INFO* model_info, MODEL_INFO* src_model_info);
int rkdemo_query_model_info(MODEL_INFO* model_info);


int rkdemo_init_input_buffer(MODEL_INFO* model_info, int node_index, API_TYPE api_type, uint8_t pass_through, rknn_tensor_type dtype, rknn_tensor_format layout_fmt);

int rkdemo_init_output_buffer(MODEL_INFO* model_info, int node_index, API_TYPE api_type, uint8_t want_float);

int rkdemo_init_input_buffer_all(MODEL_INFO* model_info, API_TYPE default_api_type, rknn_tensor_type default_t_type);


int rkdemo_init_output_buffer_all(MODEL_INFO* model_info, API_TYPE default_api_type, uint8_t default_want_float);



// order is: x -> model_top -> model_bottom -> result
int rkdemo_connect_models_node(MODEL_INFO* model_top, int top_out_index, MODEL_INFO* model_bottom, int bottom_in_index);


int rkdemo_release(MODEL_INFO* model_info);


// for native process
int offset_nchw_2_nc1hwc2(rknn_tensor_attr *src_attr, rknn_tensor_attr *native_attr, int offset, bool batch);


int rkdemo_query_dynamic_input(MODEL_INFO* model_info);

int rkdemo_reset_dynamic_input(MODEL_INFO* model_info, int dynamic_shape_group_index);

int rkdemo_thread_init(MODEL_INFO** model_infos, int num);

#endif // _RKNN_DEMO_UTILS_H