// Copyright (c) 2022 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

/*
 * @Author: Randall Zhuo
 * @Date: 2022-09-29 09:55:40
 * @LastEditors: Randall
 * @LastEditTime: 2022-09-29 09:55:41
 * @Description: TODO
 * /
 
/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <math.h>
#define _BASETSD_H
#include <ctype.h>
#include <algorithm>

#include <float.h>
#include <type_half.h>

#include "rknn_api.h"
#include <timer.h>
#include <bpe_tools.h>
#include <rknn_demo_utils.h>

#define _CNPY_ENABLE
#ifdef _CNPY_ENABLE
#include "cnpy.h"
#endif

// please set according to the your model

#define HEAD_NUM 4
#define EMBEDDING_DIM 256

#define NUM_WORD 36808
#define MAX_SENTENCE_LEN 64
#define DECODER_LAYER_NUM 3
// #define NUM_WORD 23048
// #define MAX_SENTENCE_LEN 200
// #define DECODER_LAYER_NUM 6

#define ENCODER_DICT_LEN 36808
#define DECODER_DICT_LEN 36808
#define POS_LEN 1026

#define ENCODER_INPUT_TOKEN_RIGHTSIDE_ALIGN

#define INPUT_WORD_LINE_MAX 512
#define MAX_USER_INPUT_LEN 64

extern int g_strcmp_count=0;

/*-------------------------------------------
                  Functions
-------------------------------------------*/

#ifdef _CNPY_ENABLE
static void save_npy(const char* output_path, float* output_data, rknn_tensor_attr* output_attr)
{
  std::vector<size_t> output_shape;

  for (uint32_t i = 0; i < output_attr->n_dims; ++i) {
    // output_shape.push_back(output_attr->dims[output_attr->n_dims - i - 1]); // toolkit1 is inverse
    output_shape.push_back(output_attr->dims[i]); // toolkit 2
  }

  cnpy::npy_save<float>(output_path, output_data, output_shape);
}
#endif

void safe_flush()
    {
        char c;
        while((c = getchar()) != '\n' && c != EOF);
    }


int token_embeding(float *token_embed, float *position_embed, int *tokens, int len, float *embedding){
    float scale = sqrt(EMBEDDING_DIM);
    int pad = 1;
    for (int i = 0; i < len; i++){
        for (int j = 0; j < EMBEDDING_DIM; j++){
            embedding[i * EMBEDDING_DIM + j] = token_embed[tokens[i] * EMBEDDING_DIM + j] * scale;
        }
    }

    for (int i = 0; i < len; i++){
        if (tokens[i] != 1){
            pad++;
        }
        else{
            pad = 1;
        }
        for (int j = 0; j < EMBEDDING_DIM; j++){
            embedding[i * EMBEDDING_DIM + j] += position_embed[EMBEDDING_DIM * pad + j];
        }
    }
    return 0;
}


// 1x4x16x64 -> 1x15x64x4, nchw -> nhwc
int preprocess_prev_key_value(float *prev_data, float *src_output_data, int decoder_len) {
    float mid_data[decoder_len*EMBEDDING_DIM];

    // 1x4x16x64->1x16x64x4
    for (int s=0; s<decoder_len*EMBEDDING_DIM/HEAD_NUM; s++) {
        for (int h=0; h<HEAD_NUM; h++) {
            mid_data[s*HEAD_NUM+h] = src_output_data[h*decoder_len*EMBEDDING_DIM/HEAD_NUM+s];
        }
    }

    // 1x16x64x4->1x15x64x4
    memcpy(prev_data, mid_data+EMBEDDING_DIM, (decoder_len-1)*EMBEDDING_DIM*sizeof(float));

    return 0;
}


typedef struct _NMT_TOKENS{
    float *enc_token_embed;
    float *enc_pos_embed;
    float *dec_token_embed;
    float *dec_pos_embed;
} NMT_TOKENS;

int nmt_process(MODEL_INFO *enc, 
                int encoder_len,
                MODEL_INFO *dec, 
                int decoder_len,
                NMT_TOKENS *nmt_tokens,
                int* input_token,
                int* output_token,
                TIMER *timer)
{
    int ret = 0;
    // share max buffer
    float enc_embedding[encoder_len * EMBEDDING_DIM];
    float dec_embedding[decoder_len * EMBEDDING_DIM];
    float enc_mask[encoder_len];
    float dec_mask[decoder_len];
    int input_token_sorted[encoder_len];
    memset(enc_embedding, 0x00, sizeof(enc_embedding));
    memset(dec_embedding, 0x00, sizeof(dec_embedding));
    memset(enc_mask, 0x00, sizeof(enc_mask));
    memset(dec_mask, 0x00, sizeof(dec_mask));

    // init prev key
    float prev_key[DECODER_LAYER_NUM][(decoder_len-1) * EMBEDDING_DIM];
    float prev_value[DECODER_LAYER_NUM][(decoder_len-1) * EMBEDDING_DIM];
    memset(prev_key, 0x00, sizeof(prev_key));
    memset(prev_value, 0x00, sizeof(prev_value));

    int input_token_give = 0;
    for (int i=0; i<encoder_len; i++){
        if (input_token[i] <= 1){
            break;
        }
        input_token_give++;
    }
#ifdef ENCODER_INPUT_TOKEN_RIGHTSIDE_ALIGN
    // working as [22,33,1,1,1,1] -> [1,1,1,22,33,2]
    memset(input_token_sorted, 0, encoder_len*sizeof(int));
    input_token_sorted[encoder_len-1] = 2;
    for (int i=0; i<input_token_give; i++){
        input_token_sorted[encoder_len-1 - input_token_give +i] = input_token[i];
    }
#else
    // working as [22,33,1,1,1,1] -> [22,33,2,1,1,1]
    input_token_sorted[token_list_len] = 2;
#endif

    // gen encoder mask
    printf("input tokens(all should > 0):\n");
    for (int i=0; i< encoder_len; i++){
        if (input_token_sorted[i] == 0){
            input_token_sorted[i] = 1;
            enc_mask[i] = 1;
        }
        else if(input_token_sorted[i] == 1){
            enc_mask[i] = 1;         
        }
        else{
            enc_mask[i] = 0;
        }
        printf(" %d", input_token_sorted[i]);
    }
    printf("\n");

    // printf("encoder mask:\n");
    // for (int i=0; i<encoder_len; i++){
    //     printf(" %.1f", enc_mask[i]);
    // }
    // printf("\n");

    // expand_encoder_mask
    float enc_mask_expand[encoder_len * encoder_len];
    memset(enc_mask_expand, 0x00, sizeof(enc_mask_expand));
    for (int i=0; i<encoder_len; i++){
        for (int j=0; j<encoder_len; j++){
            enc_mask_expand[i*encoder_len+j] = enc_mask[j];
        }
    }

    printf("encoder processing\n");
    timer->tik();
    token_embeding(nmt_tokens->enc_token_embed, nmt_tokens->enc_pos_embed, input_token_sorted, encoder_len, enc_embedding);
    printf("token_embeding success\n");
    float_to_half_array(enc_embedding, (half*)(enc->input_mem[0]->virt_addr), enc->in_attr[0].n_elems);
    float_to_half_array(enc_mask_expand, (half*)(enc->input_mem[1]->virt_addr), enc->in_attr[1].n_elems);


    if (ret < 0){ printf("rknn_inputs_set fail! ret=%d\n", ret); return -1; }
    timer->tok();
    timer->record_time(ENCODER_PRE_PROCESS);

    // zero_copy_io_set
    for (int input_index=0; input_index< enc->n_input; input_index++){
        if (enc->rkdmo_input_param[input_index]._already_set == true){continue;}

        ret = rknn_set_io_mem(enc->ctx, enc->input_mem[input_index], &(enc->in_attr[input_index]));
        if (ret < 0){ printf("rknn_set_io_mem fail! ret=%d\n", ret); return -1; }
        printf("rknn_set_io_mem.input[%d] success\n", input_index);
        enc->rkdmo_input_param[input_index]._already_set = true;
    }
    for (int output_index=0; output_index< enc->n_output; output_index++){
        if (enc->rkdmo_output_param[output_index]._already_set == true){continue;}

        ret = rknn_set_io_mem(enc->ctx, enc->output_mem[output_index], &(enc->out_attr[output_index]));
        if (ret < 0){ printf("rknn_set_io_mem fail! ret=%d\n", ret); return -1; }
        printf("rknn_set_io_mem.output[%d] success\n", output_index);
        enc->rkdmo_output_param[output_index]._already_set = true;
    }
    
    // Run
    timer->tik();
    ret = rknn_run(enc->ctx, nullptr);
    if (ret < 0){ printf("rknn_run fail! ret=%d\n", ret); return -1; }
    timer->tok();
    timer->print_time("encoder run");
    timer->record_time(ENCODER_RUN);

    // Get Output
    timer->tik();

    if (ret < 0){ printf("rknn_outputs_get fail! ret=%d\n", ret); return -1; }
    timer->tok();
    timer->record_time(ENCODER_POST_PROCESS);

    // // debug
    // float* enc_output_debug = (float*)(enc->outputs[0].buf);
    // for (int n=0; n<encoder_len; n++){
    //     for (int k=0; k<3; k++){
    //         half tmp = ((half*)(enc->output_mem[0]->virt_addr))[k+EMBEDDING_DIM*n];
    //         printf("%f ", half_to_float(tmp));
    //     }
    //     printf("\n");
    // }

    printf("decoder processing\n");
    float decoder_preprocess_time = 0;
    float decoder_run_time = 0;
    float decoder_postprocess_time = 0;
    for (int i = 0; i < decoder_len; i++){
        output_token[i] = 1;
    }
    output_token[0] = 2;

    // set decoder output
    for (int output_index=0; output_index< dec->n_output; output_index++){
        if (dec->rkdmo_output_param[output_index]._already_set == true){continue;}

        if (dec->out_attr[output_index].fmt == RKNN_TENSOR_NCHW){
            // dec->out_attr[output_index].fmt = RKNN_TENSOR_NCHW;
            rknn_query(dec->ctx, RKNN_QUERY_NATIVE_NC1HWC2_OUTPUT_ATTR, &(dec->out_attr[output_index]), sizeof(dec->out_attr[output_index]));
            printf("output[%d] fmt change to NC1HWC2 FROM NCHW\n", output_index);
            rknn_destroy_mem(dec->ctx, dec->output_mem[output_index]);
            dec->output_mem[output_index] = rknn_create_mem(dec->ctx, dec->out_attr[output_index].n_elems * sizeof(half)*2);
        }
        // dump_tensor_attr(&dec->out_attr[output_index]);
        dec->rkdmo_output_param[output_index]._already_set = true;
        ret = rknn_set_io_mem(dec->ctx, dec->output_mem[output_index], &(dec->out_attr[output_index]));
        if (ret < 0){ printf("rknn_set_io_mem fail! ret=%d\n", ret); return -1; }
        printf("rknn_set_io_mem.output[%d] success\n", output_index);
        dec->rkdmo_output_param[output_index]._already_set = true;
    }

    // set decoder input
    for (int input_index=0; input_index< dec->n_input; input_index++){
        if (dec->rkdmo_input_param[input_index]._already_set == true){continue;}

        if (dec->in_attr[input_index].fmt == RKNN_TENSOR_NHWC){
            rknn_query(dec->ctx, RKNN_QUERY_NATIVE_NC1HWC2_INPUT_ATTR, &(dec->in_attr[input_index]), sizeof(dec->in_attr[input_index]));
            printf("input[%d] fmt change to NC1HWC2 FROM NHWC\n", input_index);
            // 1x4x16x64输出, nc1hwc2输出, 1x16x64x4
            // 1x4x15x64输入, nc1hwc2输入, 1x1x15x64x8
            // 这两块 buffer 无法对齐, 需要手动 memcpy, 如果 channel 改成 8, 则可以无需手动 memcpy
            // dec->input_mem[input_index] = rknn_create_mem_from_fd(dec->ctx, 
            //                                                       dec->output_mem[input_index-3]->fd,
            //                                                       dec->output_mem[input_index-3]->virt_addr, 
            //                                                       dec->in_attr[input_index].n_elems* sizeof(half), 
            //                                                       EMBEDDING_DIM*sizeof(half));
            dec->input_mem[input_index] = rknn_create_mem(dec->ctx, dec->in_attr[input_index].n_elems * sizeof(half)*2);
            dec->in_attr[input_index].pass_through = 1;
        }
        // dump_tensor_attr(&dec->in_attr[input_index]);
        dec->rkdmo_input_param[input_index]._already_set = true;
        ret = rknn_set_io_mem(dec->ctx, dec->input_mem[input_index], &(dec->in_attr[input_index]));
        if (ret < 0){ printf("rknn_set_io_mem fail! ret=%d\n", ret); return -1; }
        printf("rknn_set_io_mem.input[%d] success\n", input_index);
        dec->rkdmo_input_param[input_index]._already_set = true;
    }

    {
        printf("reset decoder input and output mem\n");
        for (int input_index = 0; input_index < dec->n_input; input_index++){
            memset(dec->input_mem[input_index]->virt_addr, 0, dec->in_attr[input_index].n_elems * sizeof(half));
        }
        for (int output_index = 0; output_index < dec->n_output; output_index++){
            memset(dec->output_mem[output_index]->virt_addr, 0, dec->out_attr[output_index].n_elems * sizeof(half));
        }
    }

    // 不随着decoder的迭代而改变的输入
    memcpy(dec->input_mem[1]->virt_addr, enc->output_mem[0]->virt_addr, enc->out_attr[0].n_elems * sizeof(half));
    memcpy(dec->input_mem[2]->virt_addr, enc->input_mem[1]->virt_addr, enc->in_attr[1].n_elems * sizeof(half));

    // decoder run
    for (int num_iter = 0; num_iter < decoder_len; num_iter++){
        timer->tik();
        token_embeding(nmt_tokens->dec_token_embed, nmt_tokens->dec_pos_embed, output_token, num_iter+1, dec_embedding);

        float_to_half_array(dec_embedding + num_iter*EMBEDDING_DIM, (half*)(dec->input_mem[0]->virt_addr), dec->in_attr[0].n_elems);

        float mask;
        for (int j = 0; j < decoder_len; j++){
            if (j >= decoder_len - 1 - num_iter){
                mask = 0;
            }
            else{
                mask = 1;
            }
            dec_mask[j] = mask;
        }
        float_to_half_array(dec_mask, (half*)(dec->input_mem[3]->virt_addr), dec->in_attr[3].n_elems);

        // debug
        // if (num_iter >= 0){
        //     for (int i=0; i< dec->out_attr[1].dims[2]; i++){
        //         for (int j=0; j< dec->out_attr[1].dims[3]; j++){
        //             for (int k=0; k< dec->out_attr[1].dims[4]; k++){
        //                 int offset = 0;
        //                 offset += i* dec->out_attr[1].dims[3]* dec->out_attr[1].dims[4];
        //                 offset += j* dec->out_attr[1].dims[4];
        //                 offset += k;
        //                 half tmp;
        //                 tmp = ((half*)(dec->output_mem[1]->virt_addr))[offset];
        //                 float tmp_fp;
        //                 tmp_fp = half_to_float(tmp);
        //                 if (tmp_fp!=0){
        //                     printf("I%d - dec->output_mem[1]->virt_addr[%d][%d][%d]=%f\n", num_iter, i, j, k, tmp_fp);
        //                 }
        //             }
        //         }
        //     }
        // }

        // incremental copy
        if (num_iter != 0) {
            // timer->tik();
            for (int i = 0; i < DECODER_LAYER_NUM*2; i++) {
                memset(dec->input_mem[4+i]->virt_addr, 0, dec->in_attr[4+i].n_elems * sizeof(half));
                int increament_input_index = 4+i;
                int increament_output_index = 1+i;
                for (int h=0; h < dec->in_attr[increament_input_index].dims[1]; h++){
                    for (int w=0; w < dec->in_attr[increament_input_index].dims[2]; w++){
                        int input_offset = 0;
                        int output_offset = 0;
                        int cpy_size = 0;
                        // input dims as nhwc
                        input_offset += h* dec->in_attr[increament_input_index].dims[2]* dec->in_attr[increament_input_index].dims[3];
                        input_offset += w* dec->in_attr[increament_input_index].dims[3];
                        
                        cpy_size = dec->in_attr[increament_input_index].dims[3];

                        // output dims as nc1hwc2
                        output_offset += (h+1)* dec->out_attr[increament_output_index].dims[3]* dec->out_attr[increament_output_index].dims[4];
                        output_offset += w* dec->out_attr[increament_output_index].dims[4];

                        input_offset = input_offset * sizeof(half);
                        output_offset = output_offset * sizeof(half);
                        cpy_size = cpy_size * sizeof(half);
                        memcpy((char*)dec->input_mem[increament_input_index]->virt_addr + input_offset, 
                            (char*)dec->output_mem[increament_output_index]->virt_addr + output_offset, 
                            cpy_size);
                    }
                }
            }
            // timer->tok();
            // timer->print_time("decoder increament memcpy");
        }

        timer->tok();
        decoder_preprocess_time += timer->get_time();

        // Run
        timer->tik();
        ret = rknn_run(dec->ctx, nullptr);
        if (ret < 0){ printf("rknn_run fail! ret=%d\n", ret); return -1; }
        timer->tok();
        if (num_iter==0){timer->print_time("decoder run once");}
        decoder_run_time += timer->get_time();

        // Get Output
        timer->tik();

        // debug
        // printf("decoder output[0]\n");
        // for (int i = 0; i < 10; i++){
        //     printf("%f ", half_to_float(((half*)dec->output_mem[0]->virt_addr)[i]));
        // }

        // argmax
        int max = 0;
        half* decoder_result_array = (half*)dec->output_mem[0]->virt_addr;
        float value = half_to_float(decoder_result_array[0]);
        for (int index = 1; index < DECODER_DICT_LEN; index++){
            if (half_to_float(decoder_result_array[index]) > value){
                value = half_to_float(decoder_result_array[index]);
                max = index;
            }
        }
        //debug
        // printf("argmax - index %d, value %f\n", max, value);
        output_token[num_iter + 1] = max;
        timer->tok();
        decoder_postprocess_time += timer->get_time();
        if (max == 2){ break;}

    }

    // for debug
    int output_len=0;
    printf("decoder output token: ");
    for (int i = 0; i < decoder_len; i++){
        if (output_token[i] == 1){break;}
        printf("%d ", output_token[i]);
        output_len ++;
    }
    printf("\n");

    timer->record_time(DECODER_PRE_PROCESS, decoder_preprocess_time);
    timer->record_time(DECODER_RUN, decoder_run_time);
    timer->record_time(DECODER_POST_PROCESS, decoder_postprocess_time);

    printf("decoder processing done\n");
    return output_len;
}

int load_bin_fp32(const char* filename, float* data, int len)
{
    FILE *fp_token_embed = fopen(filename, "rb");
    if (fp_token_embed != NULL) {
        fread(data, sizeof(float), len, fp_token_embed);
        fclose(fp_token_embed);
    } else {
        printf("Open %s fail!\n", filename);
        return -1;   
    }
    return 0;
}

int read_user_input(char** words_read, int sentence_len, int word_len){
    for (int i = 0; i < sentence_len; i++)
    {
        memset(words_read[i], 0, word_len);
    }

    char input_sentence[sentence_len*word_len];
    rewind(stdin);
    printf("请输入需要翻译的英文,输入q退出:\n");
    scanf("%[^\n]", input_sentence);
    safe_flush();

    printf("===================================\n");
    int num_word = 0;
    int c = 0;
    for (int i = 0; i < MAX_USER_INPUT_LEN; i++)
    {
        if (input_sentence[i] == ' ')
        {
            num_word++;
            c = 0;
            i++;
        }
        words_read[num_word][c] = input_sentence[i];
        c++;
    }

    if (num_word==0 && strcmp(words_read[0], "q")==0){
        return -1;
    }
    return num_word;
}

int decoder_token_2_word(int* output_token, Bpe_Tools* bpe_tools)
{
    char predict_word[MAX_USER_INPUT_LEN];
    for (int i = 1; i < MAX_SENTENCE_LEN; i++)
    {
        memset(predict_word, 0x00, sizeof(predict_word));
        if (output_token[i] == 2 or output_token[i] <= 0)
        {
            break;
        }
        bpe_tools->get_word_by_token(output_token[i], predict_word);
        for (int j = 0; j < MAX_USER_INPUT_LEN; j++)
        {
            if (predict_word[j] == '@' and predict_word[j + 1] == '@')
            {
                predict_word[j] = 0;
                predict_word[j + 1] = 0;
                break;
            }
        }
        printf("%s", predict_word);
    }
    printf("\n");
    printf("===================================\n");
    return 0;
}

/*-------------------------------------------
                  Main Functions
-------------------------------------------*/
int main(int argc, char **argv)
{
    int ret;
    TIMER timer = TIMER();
    TIMER timer_global = TIMER();

    MODEL_INFO encoder_info, decoder_info;

    if (argc != 3)
    {
        printf("Usage: %s <rknn encoder model> <rknn decoder model> \n", argv[0]);
        return -1;
    }

    NMT_TOKENS nmt_tokens;
    nmt_tokens.enc_token_embed = (float*)malloc(ENCODER_DICT_LEN* EMBEDDING_DIM * sizeof(float));
    nmt_tokens.enc_pos_embed = (float*)malloc(POS_LEN* EMBEDDING_DIM * sizeof(float));
    load_bin_fp32("model/token_embed.bin", nmt_tokens.enc_token_embed, ENCODER_DICT_LEN* EMBEDDING_DIM);
    load_bin_fp32("model/position_embed.bin", nmt_tokens.enc_pos_embed, POS_LEN* EMBEDDING_DIM);

    nmt_tokens.dec_token_embed = nmt_tokens.enc_token_embed;
    nmt_tokens.dec_pos_embed = nmt_tokens.enc_pos_embed;

    encoder_info.m_path = argv[1];
    decoder_info.m_path = argv[2];

    rkdemo_init(&encoder_info);
    rkdemo_init(&decoder_info);

    // rkdemo_init_input_buffer_all(&encoder_info, NORMAL_API, RKNN_TENSOR_FLOAT32);
    // rkdemo_init_output_buffer_all(&encoder_info, NORMAL_API, 1);

    // rkdemo_init_input_buffer_all(&decoder_info, NORMAL_API, RKNN_TENSOR_FLOAT32);
    // rkdemo_init_output_buffer_all(&decoder_info, NORMAL_API, 1);

    printf("\ninit ENCODER INPUT:\n");
    rkdemo_init_input_buffer_all(&encoder_info, ZERO_COPY_API, RKNN_TENSOR_FLOAT16);
    printf("\ninit ENCODER OUTPUT:\n");
    rkdemo_init_output_buffer_all(&encoder_info, ZERO_COPY_API, 0);

    printf("\ninit DECODER INPUT:\n");
    rkdemo_init_input_buffer_all(&decoder_info, ZERO_COPY_API, RKNN_TENSOR_FLOAT16);
    printf("\ninit DECODER OUTPUT:\n");
    rkdemo_init_output_buffer_all(&decoder_info, ZERO_COPY_API, 0);


    // init dict and bpe
    Bpe_Tools* bpe_tools = new Bpe_Tools();
    // printf("create bpe\n");
    // bpe_tools->prepare_bpe_data("./model/bpe_order.txt", ORDERED);
    // printf("create token dict\n");
    // bpe_tools->prepare_token_data("./model/dict_order.txt", ORDERED);
    // bpe_tools->set_token_offset(4);
    // bpe_tools->prepare_common_word_data("./model/cw_token_map_order.txt", CW_TOKEN);

    printf("create bpe\n");
    bpe_tools->prepare_bpe_data("./model/bpe.txt", NON_ORDERED);
    printf("create token dict\n");
    bpe_tools->prepare_token_data("./model/dict.txt", NON_ORDERED);
    bpe_tools->set_token_offset(4);

    char* input_word[MAX_USER_INPUT_LEN];
    for (int i = 0; i < MAX_USER_INPUT_LEN; i++)
    {
        input_word[i] = (char*)malloc(MAX_USER_INPUT_LEN);
    }

    int loop_time = 1;
    while (1)
    {
        int num_word = read_user_input(input_word, MAX_USER_INPUT_LEN, MAX_USER_INPUT_LEN);
        if (num_word == -1)
        {
            break;
        }

        int _lt = 0;
        while (_lt < loop_time)
        {
            printf("bpe processing\n");
            _lt += 1;
            timer_global.tik();
            int num_char = 1;
            int token_list[100];
            int token_list_len=0;
            memset(token_list, 0, sizeof(token_list));
            for (int i = 0; i <= num_word; i++)
            {
                int word_tokens[WORD_LEN_LIMIT];
                int _tk_len = 0;
                _tk_len = bpe_tools->bpe_and_tokenize(input_word[i], word_tokens);
                for (int j = 0; j < _tk_len; j++)
                {
                    token_list[token_list_len] = word_tokens[j];
                    token_list_len++;
                    printf("%d ", word_tokens[j]);
                }
                printf("\n");
            }

// #ifdef ENCODER_INPUT_TOKEN_RIGHTSIDE_ALIGN
//             // working as [22,33,1,1,1,1] -> [1,1,1,22,33,2]
//             int encoder_len = encoder_info.in_attr[0].dims[1];
//             int tmp_input_token[encoder_len];
//             memset(tmp_input_token, 0, encoder_len*sizeof(int));
//             tmp_input_token[encoder_len-1] = 2;
//             for (int i=0; i<token_list_len; i++){
//                 tmp_input_token[encoder_len-1 - token_list_len +i] = token_list[i];
//             }
//             memcpy(token_list, tmp_input_token, encoder_len*sizeof(int));
// #else
//             // working as [22,33,1,1,1,1] -> [22,33,2,1,1,1]
//             token_list[token_list_len] = 2;
// #endif

            if (token_list_len > MAX_SENTENCE_LEN)
            {
                printf("\nWARNING: token_len(%d) > MAX_SENTENCE_LEN(%d), only keep %d tokens!\n", token_list_len, MAX_SENTENCE_LEN, MAX_SENTENCE_LEN);
                printf("Tokens all:");
                for (int i = 0; i < token_list_len; i++){printf(" %d", token_list[i]);}
                printf("\n");
                token_list_len = MAX_SENTENCE_LEN;
                printf("Tokens remains: ");
                for (int i = 0; i < token_list_len; i++){printf(" %d", token_list[i]);}
                printf("\n");
            }
            timer_global.tok();
            timer_global.print_time("bpe processing");

            int output_token[MAX_SENTENCE_LEN] = {0};
            int output_len = 0;
            timer_global.tik();
            printf("Start NMT-1 process\n");
            output_len = nmt_process(&encoder_info,
                                encoder_info.in_attr[0].dims[1], 
                                &decoder_info, 
                                decoder_info.in_attr[3].dims[1],
                                &nmt_tokens, 
                                token_list,
                                output_token,
                                &timer);
            timer_global.tok();
            timer_global.print_time("nmt total time");
            printf("===================================\n");

            timer_global.tik();
            decoder_token_2_word(output_token, bpe_tools);
            timer_global.tok();
            timer_global.print_time("token to word processing");

        }
    }


Release:
    // Release
    rkdemo_release(&encoder_info);
    rkdemo_release(&decoder_info);
    delete bpe_tools;

    return 0;
}
