#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <assert.h>

// multi thread support
#include <iostream>
#include <thread>
#include <vector>

#include "rknn_api.h"
#include "rknn_demo_utils.h"



// static void dump_tensor_attr(rknn_tensor_attr *attr)
void dump_tensor_attr(rknn_tensor_attr *attr)
{
    char dims_str[100];
    char temp_str[100];
    memset(dims_str, 0, sizeof(dims_str));
    for (int i = 0; i < attr->n_dims; i++) {
        strcpy(temp_str, dims_str);
        sprintf(dims_str, "%s%d,", temp_str, attr->dims[i]);
    }

    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, size_with_stride=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, dims_str,
           attr->n_elems, attr->size, attr->size_with_stride, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);

}


int rkdemo_init(MODEL_INFO* model_info){
    if (model_info->m_path == nullptr){
        printf("ERROR model path is null");
        return -1;
    }

    int ret = 0;
    ret = rknn_init(&model_info->ctx, (void *)model_info->m_path, 0, model_info->init_flag, NULL);

    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }
    ret = rkdemo_query_model_info(model_info);
    return ret;
}

int rkdemo_init_share_weight(MODEL_INFO* model_info, MODEL_INFO* src_model_info){
    if (model_info->m_path == nullptr){
        printf("ERROR model path is null");
        return -1;
    }

    int ret = 0;
    rknn_init_extend extend;
    memset(&extend,0,sizeof(rknn_init_extend));
    extend.ctx = src_model_info->ctx;

    ret = rknn_init(&model_info->ctx, (void *)model_info->m_path, 0, RKNN_FLAG_SHARE_WEIGHT_MEM, &extend);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    ret = rkdemo_query_model_info(model_info);
    return ret;
}


int rkdemo_query_model_info(MODEL_INFO* model_info){
    if (model_info->verbose_log==true) { printf("rkdemo_query_model_info");}
    int ret=0;

    rknn_input_output_num io_num;
    ret = rknn_query(model_info->ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    if (model_info->verbose_log==true) { printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output); }
    model_info->n_input = io_num.n_input;
    model_info->n_output = io_num.n_output;


    model_info->inputs = (rknn_input*)malloc(sizeof(rknn_input) * model_info->n_input);
    model_info->in_attr = (rknn_tensor_attr*)malloc(sizeof(rknn_tensor_attr) * model_info->n_input);
    model_info->in_attr_native = (rknn_tensor_attr*)malloc(sizeof(rknn_tensor_attr) * model_info->n_input);
    model_info->input_mem = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * model_info->n_input);
    model_info->rkdmo_input_param = (RKDEMO_INPUT_PARAM*)malloc(sizeof(RKDEMO_INPUT_PARAM) * model_info->n_input);
    for (int i = 0; i < model_info->n_input; i++)
    {
        memset(&(model_info->inputs[i]), 0, sizeof(rknn_input));
        memset(&(model_info->rkdmo_input_param[i]), 0, sizeof(RKDEMO_INPUT_PARAM));
    }
    
    model_info->outputs = (rknn_output*)malloc(sizeof(rknn_output) * model_info->n_output);
    model_info->out_attr = (rknn_tensor_attr*)malloc(sizeof(rknn_tensor_attr) * model_info->n_output);
    model_info->out_attr_native = (rknn_tensor_attr*)malloc(sizeof(rknn_tensor_attr) * model_info->n_output);
    model_info->output_mem = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * model_info->n_output);
    model_info->rkdmo_output_param = (RKDEMO_OUTPUT_PARAM*)malloc(sizeof(RKDEMO_OUTPUT_PARAM) * model_info->n_output);
    for (int i = 0; i < model_info->n_output; i++)
    {
        memset(&(model_info->outputs[i]), 0, sizeof(rknn_output));
        memset(&(model_info->rkdmo_output_param[i]), 0, sizeof(RKDEMO_OUTPUT_PARAM));
    }

    if (model_info->verbose_log==true) { printf("INPUTS:\n"); }
    for (int i = 0; i < model_info->n_input; i++)
    {
        model_info->in_attr[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_INPUT_ATTR, &(model_info->in_attr[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        if (model_info->verbose_log==true) {dump_tensor_attr(&model_info->in_attr[i]);}

#ifdef RKDMO_NPU_2_NATIVE_ZP
        model_info->in_attr_native[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_INPUT_ATTR, &(model_info->in_attr_native[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        if (model_info->verbose_log==true) { 
            printf("native input attr: size_with_stride %d;\n", model_info->in_attr_native[i].size_with_stride);
            printf("native input attr: w_stride %d;\n", model_info->in_attr_native[i].w_stride);
            printf("native input attr: width %d;\n", model_info->in_attr_native[i].dims[2]);
            dump_tensor_attr(&model_info->in_attr_native[i]);
        }
#endif

    }

    if (model_info->verbose_log==true) { printf("OUTPUTS:\n");}
    for (int i = 0; i < model_info->n_output; i++)
    {
        model_info->out_attr[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_OUTPUT_ATTR, &(model_info->out_attr[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        if (model_info->verbose_log==true) { dump_tensor_attr(&model_info->out_attr[i]); }

#ifdef RKDMO_NPU_2_NATIVE_ZP
        model_info->out_attr_native[i].index = i;
        // ret = rknn_query(model_info->ctx, RKNN_QUERY_NATIVE_NC1HWC2_OUTPUT_ATTR, &(model_info->out_attr_native[i]), sizeof(rknn_tensor_attr));
        ret = rknn_query(model_info->ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(model_info->out_attr_native[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        if (model_info->verbose_log==true) { dump_tensor_attr(&model_info->out_attr_native[i]);}
#endif
    }

    if (model_info->init_flag > 0){
        rknn_query(model_info->ctx, RKNN_QUERY_MEM_SIZE, &model_info->mem_size, sizeof(model_info->mem_size));
    }

    return 0;
}


int rkdemo_get_type_size(rknn_tensor_type type){
    switch (type)
    {
    case RKNN_TENSOR_FLOAT32: return 4;
    case RKNN_TENSOR_FLOAT16: return 2;
    case RKNN_TENSOR_UINT8: return 1;
    case RKNN_TENSOR_INT8: return 1;
    default: printf("ERROR: not support tensor type %s", get_type_string(type)); return -1;
    }
}


int rkdemo_init_input_buffer(MODEL_INFO* model_info, int node_index, API_TYPE api_type, uint8_t pass_through, rknn_tensor_type dtype, rknn_tensor_format layout_fmt){
    if (model_info->rkdmo_input_param[node_index]._already_init){
        printf("ERROR model input buffer already init\n");
        return -1;
    }
    model_info->rkdmo_input_param[node_index]._already_init = true;
    model_info->rkdmo_input_param[node_index].api_type = api_type;
    int elem_size = rkdemo_get_type_size(dtype);

    if (api_type==NORMAL_API){
        model_info->inputs[node_index].index = node_index;
        model_info->inputs[node_index].pass_through = pass_through;
        model_info->inputs[node_index].type = dtype;
        model_info->inputs[node_index].fmt = layout_fmt;
        model_info->inputs[node_index].size = model_info->in_attr[node_index].n_elems* elem_size;
        if (model_info->verbose_log==true) { 
            printf("rkdemo_init_input_buffer: node_index=%d, size=%d, n_elems=%d, fmt=%s, type=%s\n", 
                node_index, model_info->inputs[node_index].size, model_info->in_attr[node_index].n_elems, 
                get_format_string(layout_fmt), 
                get_type_string(dtype));
        }
    }
    else if (api_type==ZERO_COPY_API){
        // assert(dtype==RKNN_TENSOR_UINT8);
        // assert((model_info->in_attr[node_index].type==RKNN_TENSOR_UINT8) || (model_info->in_attr[node_index].type==RKNN_TENSOR_INT8));
        model_info->in_attr[node_index].fmt = layout_fmt;
        model_info->in_attr[node_index].type = dtype;

        if (layout_fmt == RKNN_TENSOR_UNDEFINED){
            model_info->input_mem[node_index] = rknn_create_mem(model_info->ctx, model_info->in_attr[node_index].size);
        }
        else{
            model_info->input_mem[node_index] = rknn_create_mem(model_info->ctx, model_info->in_attr[node_index].size_with_stride);
        }

        if (model_info->verbose_log==true) { 
            printf("rkdemo_init_input_buffer(zero copy): node_index=%d, size %d, size with stride %d, fmt=%s, type=%s\n", 
                node_index, 
                model_info->in_attr[node_index].size,
                model_info->in_attr[node_index].size_with_stride,
                get_format_string(layout_fmt),
                get_type_string(dtype));
        }
    }
    return 0;
}


int rkdemo_init_output_buffer(MODEL_INFO* model_info, int node_index, API_TYPE api_type, uint8_t want_float){
    if (model_info->rkdmo_output_param[node_index]._already_init){
        printf("ERROR model output buffer already init");
        return -1;
    }
    model_info->rkdmo_output_param[node_index]._already_init = true;
    model_info->rkdmo_output_param[node_index].api_type = api_type;

    if (api_type==NORMAL_API){
        model_info->outputs[node_index].index = node_index;
        model_info->outputs[node_index].want_float = want_float;
        if (model_info->verbose_log==true) { printf("rkdemo_init_output_buffer: node_index=%d, want_float=%d\n", node_index, want_float);}
    }
    else if(api_type==ZERO_COPY_API){
        // assert(want_float==0); // toolkit1 not support, toolkit2 support zero_copy as float output

#ifdef RKDMO_NPU_2_NATIVE_ZP
        if (want_float){
            model_info->out_attr_native[node_index].type = RKNN_TENSOR_FLOAT32;
        }
        // model_info->out_attr_native[node_index].fmt = RKNN_TENSOR_NHWC;
        // model_info->out_attr_native[node_index].fmt = RKNN_TENSOR_NC1HWC2;
        int elem_size = rkdemo_get_type_size(model_info->out_attr_native[node_index].type);
        model_info->output_mem[node_index] = rknn_create_mem(model_info->ctx, model_info->out_attr_native[node_index].n_elems*elem_size);
        if (model_info->verbose_log==true) { 
            printf("size with stride %d\n", model_info->out_attr_native[node_index].size_with_stride);
            printf("size %d\n", model_info->out_attr_native[node_index].size);
        }
#else
        if (want_float){
            model_info->out_attr[node_index].type = RKNN_TENSOR_FLOAT32;
        }
        int elem_size = rkdemo_get_type_size(model_info->out_attr[node_index].type);
        model_info->output_mem[node_index] = rknn_create_mem(model_info->ctx, model_info->out_attr[node_index].n_elems*elem_size);
        if (model_info->verbose_log==true) { 
            printf("rkdemo_init_output_buffer(zero copy): node_index=%d, size with stride %d\n", node_index, model_info->out_attr[node_index].size);
        }
#endif
    }
    return 0;
}


int rkdemo_init_input_buffer_all(MODEL_INFO* model_info, API_TYPE default_api_type, rknn_tensor_type default_t_type){
    rknn_tensor_format default_layout_fmt = RKNN_TENSOR_NHWC;
    uint8_t default_pass_through = 0;

    for (int i = 0; i < model_info->n_input; i++){
        if (model_info->rkdmo_input_param[i]._already_init){
            printf("model input buffer already init, ignore\n");
            continue;
        }

        if (model_info->rkdmo_input_param[i].enable){
            rkdemo_init_input_buffer(model_info, 
                                     i, 
                                     model_info->rkdmo_input_param[i].api_type, 
                                     model_info->rkdmo_input_param[i].pass_through, 
                                     model_info->rkdmo_input_param[i].dtype, 
                                     model_info->rkdmo_input_param[i].layout_fmt);
        }
        else{
            if (model_info->in_attr[i].n_dims==4){
                default_layout_fmt = model_info->in_attr[i].fmt;

            }
            
            rkdemo_init_input_buffer(model_info, i, default_api_type, default_pass_through, default_t_type, default_layout_fmt);
        }
    }
    return 0;
}


int rkdemo_init_output_buffer_all(MODEL_INFO* model_info, API_TYPE default_api_type, uint8_t default_want_float){
    for (int i = 0; i < model_info->n_output; i++){
        if (model_info->rkdmo_output_param[i]._already_init){
            printf("model output buffer already init, ignore");
            continue;
        }

        if (model_info->rkdmo_output_param[i].enable){
            rkdemo_init_output_buffer(model_info, i, model_info->rkdmo_output_param[i].api_type, model_info->rkdmo_output_param[i].want_float);
        }
        else{
            rkdemo_init_output_buffer(model_info, i, default_api_type, default_want_float);
        }
    }
    return 0;
}



int rkdemo_reset_all_buffer(MODEL_INFO* model_info){
    for (int i = 0; i < model_info->n_input; i++)
    {
        if (model_info->input_mem[i] != nullptr){
            rknn_destroy_mem(model_info->ctx, model_info->input_mem[i]);
        }
        memset(model_info->inputs, 0, sizeof(rknn_input)* model_info->n_input);
        memset(model_info->in_attr, 0, sizeof(rknn_tensor_attr)* model_info->n_input);
        memset(model_info->in_attr_native, 0, sizeof(rknn_tensor_attr)* model_info->n_input);
        memset(model_info->input_mem, 0, sizeof(rknn_tensor_mem)* model_info->n_input);
        memset(model_info->rkdmo_input_param, 0, sizeof(RKDEMO_INPUT_PARAM)* model_info->n_input);
    }

    for (int i = 0; i < model_info->n_output; i++){
        if (model_info->output_mem[i] != nullptr){
            rknn_destroy_mem(model_info->ctx, model_info->output_mem[i]);
        }
        memset(model_info->outputs, 0, sizeof(rknn_output)* model_info->n_output);
        memset(model_info->out_attr, 0, sizeof(rknn_tensor_attr)* model_info->n_output);
        memset(model_info->out_attr_native, 0, sizeof(rknn_tensor_attr)* model_info->n_output);
        memset(model_info->output_mem, 0, sizeof(rknn_tensor_mem)* model_info->n_output);
        memset(model_info->rkdmo_output_param, 0, sizeof(RKDEMO_OUTPUT_PARAM)* model_info->n_output);

    }
    return 0;
}

static void dump_input_dynamic_range(rknn_input_range *dyn_range)
{
    std::string range_str = "";
    for (int n = 0; n < dyn_range->shape_number; ++n)
    {
        range_str += n == 0 ? "[" : ",[";
        range_str += dyn_range->n_dims < 1 ? "" : std::to_string(dyn_range->dyn_range[n][0]);
        for (int i = 1; i < dyn_range->n_dims; ++i)
        {
            range_str += ", " + std::to_string(dyn_range->dyn_range[n][i]);
        }
        range_str += "]";
    }

    printf("  index=%d, name=%s, shape_number=%d, range=[%s], fmt = %s\n", dyn_range->index, dyn_range->name,
           dyn_range->shape_number, range_str.c_str(), get_format_string(dyn_range->fmt));
}


int rkdemo_query_dynamic_input(MODEL_INFO* model_info){
    if (model_info->verbose_log==true) { printf("dynamic inputs shape range:\n"); }
    model_info->dyn_range = (rknn_input_range*)malloc(model_info->n_input * sizeof(rknn_input_range));
    memset(model_info->dyn_range, 0, model_info->n_input * sizeof(rknn_input_range));
    
    int ret = 0;
    for (uint32_t i = 0; i < model_info->n_input; i++)
    {
        model_info->dyn_range[i].index = i;
        ret = rknn_query(model_info->ctx, RKNN_QUERY_INPUT_DYNAMIC_RANGE, &model_info->dyn_range[i], sizeof(rknn_input_range));
        if (ret != RKNN_SUCC)
        {
            // fprintf(stderr, "rknn_query error! ret=%d\n", ret);
            printf("model may has no dynamic shape info\n");
            model_info->is_dyn_shape = false;
            return -1;
        }
        if (model_info->verbose_log==true) { dump_input_dynamic_range(&model_info->dyn_range[i]); }
    }

    for (int i=0; i < model_info->n_input; i++){
        if (model_info->diff_input_idx > 0){ break;}

        for (int d=0; d < model_info->dyn_range[i].n_dims; d++){
            if (model_info->diff_input_idx > 0){ break;}
            int compare_dim = -1;

            for (int j=0; j < model_info->dyn_range[i].shape_number; j++){
                if (model_info->diff_input_idx > 0){ break;}
            
                if (compare_dim == -1){
                    compare_dim = model_info->dyn_range[i].dyn_range[j][d];
                }
                else if (compare_dim != model_info->dyn_range[i].dyn_range[j][d]){
                    model_info->diff_input_idx = i;
                }
            }
        }
    }

    model_info->is_dyn_shape = true;
    return 0;
}

int rkdemo_reset_dynamic_input(MODEL_INFO* model_info, int dynamic_shape_group_index){
    int ret = 0;
    if (model_info->dyn_range[0].shape_number == 0){
        printf("model has no dynamic shape info, please call 'rkdemo_query_dynamic_input' first\n");
        return -1;
    }

    if (dynamic_shape_group_index >= model_info->dyn_range[0].shape_number){
        printf("dynamic_shape_group_index - [%d] exceed [%d]\n", dynamic_shape_group_index, model_info->dyn_range[0].shape_number);
    }

    // printf("---> diff index %d\n", model_info->diff_input_idx);
    rknn_tensor_attr tmp_attr;
    tmp_attr.index = model_info->diff_input_idx;
    ret = rknn_query(model_info->ctx, RKNN_QUERY_INPUT_ATTR, &tmp_attr, sizeof(rknn_tensor_attr));
    for (int d=0; d < tmp_attr.n_dims; d++){
        tmp_attr.dims[d] = model_info->dyn_range[model_info->diff_input_idx].dyn_range[dynamic_shape_group_index][d];
    }
    // printf("--> dump set attr\n");
    // dump_tensor_attr(&tmp_attr);
    rknn_set_input_shape(model_info->ctx, &tmp_attr);

    // update input output attr
    // printf("--> requery attr\n");
    for (int i = 0; i < model_info->n_input; i++){
        // ret = rknn_query(model_info->ctx, RKNN_QUERY_CURRENT_NATIVE_INPUT_ATTR, &model_info->in_attr[i], sizeof(rknn_tensor_attr));
        // dump_tensor_attr(&model_info->in_attr[i]);
        ret = rknn_query(model_info->ctx, RKNN_QUERY_CURRENT_INPUT_ATTR, &model_info->in_attr[i], sizeof(rknn_tensor_attr));
        // if (i == tmp_attr.index) {  dump_tensor_attr(&model_info->in_attr[i]);}
    }
    for (int i = 0; i < model_info->n_output; i++){
        ret = rknn_query(model_info->ctx, RKNN_QUERY_CURRENT_OUTPUT_ATTR, &model_info->out_attr[i], sizeof(rknn_tensor_attr));
    }

    return 0;
}

int rkdemo_thread_init(MODEL_INFO** model_infos, int num){
    int ret = 0;
    std::vector<std::thread> threads;

    // 创建多个线程，并将其入口点设置为 thread_func 函数
    for (int i = 0; i < num; ++i) {
        threads.emplace_back(rkdemo_init, model_infos[i]);
    }

    // 等待所有线程执行完毕
    for (auto& t : threads) {
        t.join();
    }
    // std::cout << "Main thread exits." << std::endl;

    int init_flag = 0;
    init_flag = model_infos[0]->init_flag;
    for (int i = 1; i < num; i++){
        if (init_flag != model_infos[i]->init_flag){
            printf("model init failed, all model should have same init_flag\n");
            return -1;
        }
    }

#ifdef RKNN_INTERNAL_REUSE
    if (init_flag == RKNN_FLAG_INTERNAL_ALLOC_OUTSIDE){
        uint32_t max_internal_size = 0;
        for (int i = 0; i < num; i++){
            if (model_infos[i]->mem_size.total_internal_size > max_internal_size){
                max_internal_size = model_infos[i]->mem_size.total_internal_size;
            }
        }
        // 申请mem并放在第一个model info内
        model_infos[0]->internal_mem_max = rknn_create_mem(model_infos[0]->ctx, max_internal_size);


    // internal_mem[n] = rknn_create_mem_from_fd(ctx[n], internal_mem_max->fd,
    //                                           internal_mem_max->virt_addr, mem_size[n].total_internal_size, 0);
    // ret = rknn_set_internal_mem(ctx[n], internal_mem[n]);

        rknn_tensor_mem* mem_max;
        mem_max = model_infos[0]->internal_mem_max;
        for (int i = 0; i < num; i++){
            model_infos[i]->internal_mem_outside = rknn_create_mem_from_fd(
                model_infos[i]->ctx, mem_max->fd, mem_max->virt_addr, model_infos[i]->mem_size.total_internal_size, 0);
            ret = rknn_set_internal_mem(model_infos[i]->ctx, model_infos[i]->internal_mem_outside);
            if (ret < 0) { printf("rknn_set_internal_mem fail! ret=%d\n", ret);}
        
            printf("internal cma info: virt = %p, phy=0x%lx, fd =%d, size=%d\n", 
                    model_infos[i]->internal_mem_outside->virt_addr, 
                    model_infos[i]->internal_mem_outside->phys_addr, 
                    model_infos[i]->internal_mem_outside->fd, 
                    model_infos[i]->internal_mem_outside->size);
        }
    }
#endif

    return 0;
}


int rkdemo_release(MODEL_INFO* model_info){
    for (int i = 0; i < model_info->n_input; i++){
        if (model_info->rkdmo_input_param[i].api_type == ZERO_COPY_API){
            rknn_destroy_mem(model_info->ctx, model_info->input_mem[i]);
        }
    }

    for (int i = 0; i < model_info->n_output; i++){
        if (model_info->rkdmo_output_param[i].api_type == ZERO_COPY_API){
            rknn_destroy_mem(model_info->ctx, model_info->output_mem[i]);
        }
    }

    if (model_info->internal_mem_outside){
        rknn_destroy_mem(model_info->ctx, model_info->internal_mem_outside);
    }
    if (model_info->internal_mem_max){
        rknn_destroy_mem(model_info->ctx, model_info->internal_mem_max);
    }

    if (model_info->ctx>0){
        rknn_destroy(model_info->ctx);
    }

    free(model_info->inputs);
    free(model_info->in_attr);
    free(model_info->in_attr_native);
    free(model_info->input_mem);
    free(model_info->rkdmo_input_param);

    free(model_info->outputs);
    free(model_info->out_attr);
    free(model_info->out_attr_native);
    free(model_info->output_mem);
    free(model_info->rkdmo_output_param);

    return 0;
}
