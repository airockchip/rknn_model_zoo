// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <math.h>
#define _BASETSD_H
#include <ctype.h>
#include <algorithm>

#include <float.h>
#include "type_half.h"

#include "rknn_api.h"
// #include "cnpy.h"

#include "easy_timer.h"
#include "bpe_tools.h"
#include "rknn_demo_utils.h"
#include "lite_transformer.h"

#define USE_NORMAL_API 1

/*-------------------------------------------
                  Functions
-------------------------------------------*/

// static void save_npy(const char* output_path, float* output_data, rknn_tensor_attr* output_attr)
// {
//   std::vector<size_t> output_shape;

//   for (uint32_t i = 0; i < output_attr->n_dims; ++i) {
//     // output_shape.push_back(output_attr->dims[output_attr->n_dims - i - 1]); // toolkit1 is inverse
//     output_shape.push_back(output_attr->dims[i]); // toolkit 2
//   }

//   cnpy::npy_save<float>(output_path, output_data, output_shape);
// }


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


int sentence_to_word(const char* sentence, char** word, int max_word_num_in_sentence, int max_word_len)
{
    int num_word = 0;
    int c = 0;
    for (int i = 0; i < max_word_num_in_sentence; i++)
    {
        memset(word[i], 0, max_word_len);
    }
    for (int i = 0; i < max_word_num_in_sentence; i++)
    {
        if (sentence[i] == ' ')
        {
            num_word++;
            c = 0;
            i++;
        }
        word[num_word][c] = sentence[i];
        c++;
    }
    return num_word;
}


int decoder_token_2_word(int* output_token, char* strings, Bpe_Tools* bpe_tools)
{
    char predict_word[MAX_WORD_LEN];
    for (int i = 1; i < MAX_WORD_LEN; i++)
    {
        memset(predict_word, 0x00, sizeof(predict_word));
        if (output_token[i] == 2 or output_token[i] <= 0)
        {
            break;
        }
        bpe_tools->get_word_by_token(output_token[i], predict_word);
        for (int j = 0; j < MAX_WORD_LEN; j++)
        {
            if (predict_word[j] == '@' and predict_word[j + 1] == '@')
            {
                predict_word[j] = 0;
                predict_word[j + 1] = 0;
                break;
            }
        }
        // printf("%s", predict_word);
        strcat(strings, predict_word);
    }
    // printf("\n");
    // printf("===================================\n");
    return 0;
}

// 1x4x16x64 -> 1x15x64x4
int preprocess_prev_key_value(float *prev_data, float *save_data)
{
    float mid_data[MAX_SENTENCE_LEN * EMBEDDING_DIM];

    // 1x4x16x64->1x16x64x4
    for (int s = 0; s < MAX_SENTENCE_LEN * EMBEDDING_DIM / HEAD_NUM; s++)
    {
        for (int h = 0; h < HEAD_NUM; h++)
        {
            mid_data[s * HEAD_NUM + h] = save_data[h * MAX_SENTENCE_LEN * EMBEDDING_DIM / HEAD_NUM + s];
        }
    }

    // 1x16x64x4->1x15x64x4
    memcpy(prev_data, mid_data + EMBEDDING_DIM, (MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM * sizeof(float));

    return 0;
}

// 1x4x16x64 -> 1x15x64x4
int preprocess_prev_key_value_half(half *prev_data, half *save_data)
{
    half mid_data[MAX_SENTENCE_LEN * EMBEDDING_DIM];

    // 1x4x16x64->1x16x64x4
    for (int s = 0; s < MAX_SENTENCE_LEN * EMBEDDING_DIM / HEAD_NUM; s++)
    {
        for (int h = 0; h < HEAD_NUM; h++)
        {
            mid_data[s * HEAD_NUM + h] = save_data[h * MAX_SENTENCE_LEN * EMBEDDING_DIM / HEAD_NUM + s];
        }
    }

    // 1x16x64x4->1x15x64x4
    memcpy(prev_data, mid_data + EMBEDDING_DIM, (MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM * sizeof(half));

    half prev_mid_data[(MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM];
    memcpy(prev_mid_data, prev_data, (MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM * sizeof(half));

    // 1x1x64x4 -> 1x4x15x64
    // zero-copy need layout [1,4,15,64]
    for (int h = 0; h < HEAD_NUM; h++) {
        for (int s = 0; s < (MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM / HEAD_NUM; s++) {
            prev_data[h * (MAX_SENTENCE_LEN - 1) * EMBEDDING_DIM / HEAD_NUM + s] = prev_mid_data[s  * HEAD_NUM + h];
        }
    }

    return 0;
}

int dump_float(const float *array, int count, bool is_in, int index)
{
    // open file
    FILE *file = nullptr;

    if (is_in) {
        char name[64] = {0};
        sprintf(name, "in_%d.tensor", index);
        file = fopen(name, "w");
    } else {
        file = fopen("out.tensor", "w");
    }

    if (file == NULL)
    {
        perror("Error opening file");
        return -1; // open fail
    }

    // write float data
    for (int i = 0; i < count; ++i)
    {
        if (fprintf(file, "%f\n", array[i]) < 0)
        {
            perror("Error writing to file");
            fclose(file);
            return -1; // write fail
        }
    }

    // close file
    fclose(file);

    return 0;
}

int rknn_nmt_process(
                    rknn_lite_transformer_context_t* app_ctx,
                    int* input_token,
                    int* output_token)
{
    int ret = 0;

    TIMER timer;
    TIMER timer_total;

    // share max buffer
    float enc_embedding[app_ctx->enc_len * EMBEDDING_DIM];
    float dec_embedding[app_ctx->dec_len * EMBEDDING_DIM];
    float enc_mask[app_ctx->enc_len];
    float dec_mask[app_ctx->dec_len];
    float dec_enc_mask[app_ctx->dec_len];
    int input_token_sorted[app_ctx->enc_len];
    memset(enc_embedding, 0x00, sizeof(enc_embedding));
    memset(dec_embedding, 0x00, sizeof(dec_embedding));
    memset(enc_mask, 0x00, sizeof(enc_mask));
    memset(dec_mask, 0x00, sizeof(dec_mask));
    memset(dec_enc_mask, 0x00, sizeof(app_ctx->dec_len));

    // init prev key
    float prev_key[DECODER_LAYER_NUM][(app_ctx->dec_len-1) * EMBEDDING_DIM];
    float prev_value[DECODER_LAYER_NUM][(app_ctx->dec_len-1) * EMBEDDING_DIM];
    memset(prev_key, 0x00, sizeof(prev_key));
    memset(prev_value, 0x00, sizeof(prev_value));

    int input_token_give = 0;
    for (int i=0; i<app_ctx->enc_len; i++){
        if (input_token[i] <= 1){
            break;
        }
        input_token_give++;
    }
#ifdef ENCODER_INPUT_TOKEN_RIGHTSIDE_ALIGN
    // working as [22,33,1,1,1,1] -> [1,1,1,22,33,2]
    memset(input_token_sorted, 0, app_ctx->enc_len*sizeof(int));
    input_token_sorted[app_ctx->enc_len-1] = 2;
    for (int i=0; i<input_token_give; i++){
        input_token_sorted[app_ctx->enc_len-1 - input_token_give +i] = input_token[i];
    }
#else
    // working as [22,33,1,1,1,1] -> [22,33,2,1,1,1]
    memset(input_token_sorted, 0, app_ctx->enc_len * sizeof(int));
    for (int i = 0; i < input_token_give; i++) {
        input_token_sorted[app_ctx->enc_len - 1 - input_token_give + i] = input_token[i];
    }
    input_token_sorted[input_token_give] = 2;
#endif

    // gen encoder mask
    printf("input tokens(all should > 0):\n");
    for (int i=0; i< app_ctx->enc_len; i++){
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

    // expand_encoder_mask
    float enc_mask_expand[app_ctx->enc_len * app_ctx->enc_len];
    memset(enc_mask_expand, 0x00, sizeof(enc_mask_expand));
    for (int i=0; i<app_ctx->enc_len; i++){
        for (int j=0; j<app_ctx->enc_len; j++){
            enc_mask_expand[i*app_ctx->enc_len+j] = enc_mask[j];
        }
    }

    token_embeding(app_ctx->nmt_tokens.enc_token_embed, app_ctx->nmt_tokens.enc_pos_embed, input_token_sorted, app_ctx->enc_len, enc_embedding);
#if USE_NORMAL_API
    app_ctx->enc.inputs[0].buf = enc_embedding;
    app_ctx->enc.inputs[1].buf = enc_mask_expand;
#else
    float_to_half_array(enc_embedding, (half *)(app_ctx->enc.in_mems[0]->logical_addr), app_ctx->enc.in_attrs[0].n_elems);
    float_to_half_array(enc_mask_expand, (half *)(app_ctx->enc.in_mems[1]->logical_addr), app_ctx->enc.in_attrs[1].n_elems);
#endif

    // Run Encoder
    timer.tik();

#if USE_NORMAL_API
    ret = rknn_inputs_set(app_ctx->enc.ctx, app_ctx->enc.n_input, app_ctx->enc.inputs);
    if (ret < 0)
    {
        printf("[enc]rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }
#endif

    ret = rknn_run(app_ctx->enc.ctx, nullptr);
    if (ret < 0){ printf("[enc] rknn_run fail! ret=%d\n", ret); return -1; }

#if USE_NORMAL_API
    ret = rknn_outputs_get(app_ctx->enc.ctx, app_ctx->enc.n_output, app_ctx->enc.outputs, nullptr);
    if (ret < 0)
    {
        printf("[enc] rknn_outputs_get fail! ret=%d\n", ret);
        return -1;
    }
#endif

    timer.tok();
    timer.print_time("rknn encoder run");


#if USE_NORMAL_API
    // prev_key / prev_value use NHWC format.
    for (int i = 4; i < app_ctx->dec.n_input; i++) {
        app_ctx->dec.inputs[i].fmt = RKNN_TENSOR_NHWC;
    }
#else
    // reset memory for inputs / outputs of decoder
    for (int in_index = 0; in_index < app_ctx->dec.n_input; in_index++) {
        memset(app_ctx->dec.in_mems[in_index]->logical_addr, 0, app_ctx->dec.in_attrs[in_index].size);
    }
    for (int out_index = 0; out_index < app_ctx->dec.n_output; out_index++) {
        memset(app_ctx->dec.out_mems[out_index]->logical_addr, 0, app_ctx->dec.out_attrs[out_index].size);
    }
#endif

    // Inputs that does not change as the decoder iterates
#if USE_NORMAL_API
    app_ctx->dec.inputs[1].buf = app_ctx->enc.outputs[0].buf;
    app_ctx->dec.inputs[2].buf = enc_mask;
#else
    memcpy(app_ctx->dec.in_mems[1]->logical_addr, app_ctx->enc.out_mems[0]->logical_addr, app_ctx->enc.out_attrs[0].size);
    memcpy(app_ctx->dec.in_mems[2]->logical_addr, app_ctx->enc.in_mems[1]->logical_addr, app_ctx->dec.in_attrs[2].size);
#endif

    for (int i = 0; i < app_ctx->dec_len; i++)
    {
        output_token[i] = 1;
    }
    output_token[0] = 2;

    // decoder run
    timer_total.tik();
    for (int num_iter = 0; num_iter < app_ctx->dec_len; num_iter++){
        token_embeding(app_ctx->nmt_tokens.dec_token_embed, app_ctx->nmt_tokens.dec_pos_embed, output_token, num_iter + 1, dec_embedding);
#if USE_NORMAL_API
        app_ctx->dec.inputs[0].buf = dec_embedding + num_iter * EMBEDDING_DIM;
#else
        float_to_half_array(dec_embedding + num_iter * EMBEDDING_DIM, (half *)(app_ctx->dec.in_mems[0]->logical_addr), app_ctx->dec.in_attrs[0].n_elems);
#endif

        float mask;
        for (int j = 0; j < app_ctx->dec_len; j++) {
            if (j >= app_ctx->dec_len - 1 - num_iter) {
                mask = 0;
            } else {
                mask = 1;
            }
            dec_mask[j] = mask;
        }
#if USE_NORMAL_API
        app_ctx->dec.inputs[3].buf = dec_mask;
#else
        float_to_half_array(dec_mask, (half *)(app_ctx->dec.in_mems[3]->logical_addr), app_ctx->dec.in_attrs[3].n_elems);
#endif

        if (num_iter != 0) {
#if USE_NORMAL_API
            // copy previous output to input
            preprocess_prev_key_value(prev_key[0], (float *)app_ctx->dec.outputs[1].buf);
            preprocess_prev_key_value(prev_value[0], (float *)app_ctx->dec.outputs[2].buf);
            preprocess_prev_key_value(prev_key[1], (float *)app_ctx->dec.outputs[3].buf);
            preprocess_prev_key_value(prev_value[1], (float *)app_ctx->dec.outputs[4].buf);
            preprocess_prev_key_value(prev_key[2], (float *)app_ctx->dec.outputs[5].buf);
            preprocess_prev_key_value(prev_value[2], (float *)app_ctx->dec.outputs[6].buf);

            rknn_outputs_release(app_ctx->dec.ctx, app_ctx->dec.n_output, app_ctx->dec.outputs);
#else
            // previous key 0
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[4]->logical_addr), (half *)(app_ctx->dec.out_mems[1]->logical_addr));
            // previous value 0
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[5]->logical_addr), (half *)(app_ctx->dec.out_mems[2]->logical_addr));
            // previous key 1
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[6]->logical_addr), (half *)(app_ctx->dec.out_mems[3]->logical_addr));
            // previous value 1
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[7]->logical_addr), (half *)(app_ctx->dec.out_mems[4]->logical_addr));
            // previous key 2
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[8]->logical_addr), (half *)(app_ctx->dec.out_mems[5]->logical_addr));
            // previous value 2
            preprocess_prev_key_value_half((half *)(app_ctx->dec.in_mems[9]->logical_addr), (half *)(app_ctx->dec.out_mems[6]->logical_addr));
#endif
        }

#if USE_NORMAL_API
        app_ctx->dec.inputs[4].buf = prev_key[0];
        app_ctx->dec.inputs[5].buf = prev_value[0];
        app_ctx->dec.inputs[6].buf = prev_key[1];
        app_ctx->dec.inputs[7].buf = prev_value[1];
        app_ctx->dec.inputs[8].buf = prev_key[2];
        app_ctx->dec.inputs[9].buf = prev_value[2];

        // set inputs
        ret = rknn_inputs_set(app_ctx->dec.ctx, app_ctx->dec.n_input, app_ctx->dec.inputs);
        if (ret < 0)
        {
            printf("[decoder] rknn_inputs_set fail! ret=%d\n", ret);
            return -1;
        }
#endif

        // Run
        timer.tik();
        ret = rknn_run(app_ctx->dec.ctx, nullptr);
        timer.tok();
        if (ret < 0){ printf("rknn_run fail! ret=%d\n", ret); return -1; }
        timer.print_time("rknn decoder run");

#if USE_NORMAL_API
        // Get outputs
        ret = rknn_outputs_get(app_ctx->dec.ctx, app_ctx->dec.n_output, app_ctx->dec.outputs, NULL);
        if (ret < 0)
        {
            printf("rknn_decoder_outputs_get fail! ret=%d\n", ret);
            return -1;
        }
#endif

        // argmax
        int max = 0;
        float value;
#if USE_NORMAL_API
        float* decoder_result_array = (float*)(app_ctx->dec.outputs[0].buf);
        value = decoder_result_array[0];
        for (int index = 1; index < app_ctx->dec.out_attrs[0].dims[0]; index++){
            if (decoder_result_array[index] > value){
                value = decoder_result_array[index];
                max = index;
            }
        }
#else
        half *decoder_result_array = (half *)(app_ctx->dec.out_mems[0]->logical_addr);
        value = half_to_float(decoder_result_array[0]);
        for (int index = 1; index < app_ctx->dec.out_attrs[0].dims[0]; index++) {
            if (half_to_float(decoder_result_array[index]) > value){
                value = half_to_float(decoder_result_array[index]);
                max = index;
            }
        }
#endif
        //debug
        // printf("argmax - index %d, value %f\n", max, value);
        output_token[num_iter + 1] = max;
        if (max == 2){ break;}
    }
    timer_total.tok();

#if USE_NORMAL_API
    rknn_outputs_release(app_ctx->enc.ctx, app_ctx->enc.n_output, app_ctx->enc.outputs);
    rknn_outputs_release(app_ctx->dec.ctx, app_ctx->dec.n_output, app_ctx->dec.outputs);
#endif

    // for debug
    int output_len=0;
    printf("decoder output token: ");
    for (int i = 0; i < app_ctx->dec_len; i++){
        if (output_token[i] == 1){break;}
        printf("%d ", output_token[i]);
        output_len ++;
    }
    printf("\n");

    timer.print_time("rknn decoder once run");
    printf("decoder run %d times. ", output_len-1);
    timer_total.print_time("cost");

    return output_len;
}


int init_lite_transformer_model(const char* encoder_path,
                                const char* decoder_path,
                                const char* token_embed_path,
                                const char* pos_embed_path,
                                const char* bpe_dict_path,
                                const char* token_dict_path,
                                const char* common_word_path,
                                DICT_ORDER_TYPE dict_order_type,
                                rknn_lite_transformer_context_t* app_ctx)
{
    int ret = 0;
    memset(app_ctx, 0x00, sizeof(rknn_lite_transformer_context_t));
#if USE_NORMAL_API
    app_ctx->enc.use_zp = false;
    app_ctx->dec.use_zp = false;
#else
    app_ctx->enc.use_zp = true;
    app_ctx->dec.use_zp = true;
    printf("!!!!!!!!!!!!!!!!!!!!\n");
    printf("If you want to use the zero-copy API, please ensure that the RKNN model used is a precompiled model.\n");
    printf("You can refer to this example to precompile the model: \n");
    printf("    https://github.com/rockchip-linux/rknn-toolkit/tree/master/examples/common_function_demos/export_rknn_precompile_model\n");
    printf("!!!!!!!!!!!!!!!!!!!!\n");
#endif

    // Init encoder and decoder
    printf("--> init rknn encoder %s\n", encoder_path);
    ret = rkdemo_model_init(true, encoder_path, &(app_ctx->enc));
    if (ret < 0)
    {
        printf("init encoder failed!\n");
        return ret;
    }

    printf("--> init rknn decoder %s\n", decoder_path);
    ret = rkdemo_model_init(false, decoder_path, &(app_ctx->dec));
    if (ret < 0)
    {
        printf("init decoder failed.\n");
        return ret;
    }

    // set pre-process / post-process configure
    app_ctx->enc_len = app_ctx->enc.in_attrs[0].dims[1];
    app_ctx->dec_len = app_ctx->dec.in_attrs[1].dims[1];

    // init dict and bpe
    int nmt_word_dict_len = app_ctx->dec.out_attrs[0].n_elems/ app_ctx->dec.out_attrs[0].dims[2];
    app_ctx->nmt_tokens.enc_token_embed = (float*)malloc(nmt_word_dict_len* EMBEDDING_DIM * sizeof(float));
    app_ctx->nmt_tokens.enc_pos_embed = (float*)malloc(POS_LEN* EMBEDDING_DIM * sizeof(float));
    printf("--> load token embed: %s\n", token_embed_path);
    ret = load_bin_fp32(token_embed_path, app_ctx->nmt_tokens.enc_token_embed, nmt_word_dict_len* EMBEDDING_DIM);
    if (ret != 0){ return -1;}
    printf("--> load pos embed: %s\n", pos_embed_path);
    ret = load_bin_fp32(pos_embed_path, app_ctx->nmt_tokens.enc_pos_embed, POS_LEN* EMBEDDING_DIM);
    if (ret != 0){ return -1;}
    app_ctx->nmt_tokens.dec_token_embed = app_ctx->nmt_tokens.enc_token_embed;
    app_ctx->nmt_tokens.dec_pos_embed = app_ctx->nmt_tokens.enc_pos_embed;


    app_ctx->bpe_tools = new Bpe_Tools();
    printf("--> load bpe_dict: %s\n", bpe_dict_path);
    ret = app_ctx->bpe_tools->prepare_bpe_data(bpe_dict_path, dict_order_type);
    if (ret != 0){ return -1;}
    printf("--> load word dict: %s\n", token_dict_path);
    ret = app_ctx->bpe_tools->prepare_token_data(token_dict_path, dict_order_type);
    if (ret != 0){ return -1;}
    app_ctx->bpe_tools->set_token_offset(4);

    if (common_word_path != nullptr){
        printf("--> load common word dict: %s\n", common_word_path);
        ret = app_ctx->bpe_tools->prepare_common_word_data(common_word_path, CW_TOKEN);
        if (ret != 0){ return -1;}
    }

    return 0;
}


int release_lite_transformer_model(rknn_lite_transformer_context_t* app_ctx)
{
    // Release
    rkdemo_model_release(&app_ctx->enc);
    rkdemo_model_release(&app_ctx->dec);
    free(app_ctx->nmt_tokens.enc_token_embed);
    free(app_ctx->nmt_tokens.enc_pos_embed);
    delete app_ctx->bpe_tools;
    return 0;
}


int inference_lite_transformer_model(rknn_lite_transformer_context_t* app_ctx,
                                     const char* input_sentence,
                                     char* output_sentence)
{
    TIMER timer;
    char* input_word[MAX_WORD_NUM_IN_SENTENCE];
    char* output_word[MAX_WORD_NUM_IN_SENTENCE];
    for (int i = 0; i < MAX_WORD_NUM_IN_SENTENCE; i++)
    {
        input_word[i] = (char*)malloc(MAX_WORD_LEN);
        output_word[i] = (char*)malloc(MAX_WORD_LEN);
    }
    timer.tik();
    int num_word = sentence_to_word(input_sentence, input_word, MAX_WORD_NUM_IN_SENTENCE, MAX_WORD_LEN);

    int token_list[100];
    int token_list_len=0;
    memset(token_list, 0, sizeof(token_list));
    for (int i = 0; i <= num_word; i++)
    {
        int word_tokens[WORD_LEN_LIMIT];
        int _tk_len = 0;
        _tk_len = app_ctx->bpe_tools->bpe_and_tokenize(input_word[i], word_tokens);
        for (int j = 0; j < _tk_len; j++)
        {
            token_list[token_list_len] = word_tokens[j];
            token_list_len++;
        }
    }
    timer.tok();
    timer.print_time("bpe preprocess");

    int max_input_len = app_ctx->enc_len;
    if (token_list_len > max_input_len)
    {
        printf("\nWARNING: token_len(%d) > max_input_len(%d), only keep %d tokens!\n", token_list_len, max_input_len, max_input_len);
        printf("Tokens all     :");
        for (int i = 0; i < token_list_len; i++){printf(" %d", token_list[i]);}
        printf("\n");
        token_list_len = max_input_len;
        printf("Tokens remains :");
        for (int i = 0; i < token_list_len; i++){printf(" %d", token_list[i]);}
        printf("\n");
    }

    int output_token[max_input_len];
    memset(output_token, 0, sizeof(output_token));
    int output_len = 0;
    output_len = rknn_nmt_process(app_ctx, token_list, output_token);
    if (output_len < 0) {
        printf("rknn_nmt_process fail, please check log.\n");
        return -1;
    }

    memset(output_sentence, 0, MAX_USER_INPUT_LEN);
    int ret = 0;
    ret = decoder_token_2_word(output_token, output_sentence, app_ctx->bpe_tools);

    for (int i = 0; i < MAX_WORD_NUM_IN_SENTENCE; i++){
        free(input_word[i]);
        free(output_word[i]);
    }
    return 0;
}
