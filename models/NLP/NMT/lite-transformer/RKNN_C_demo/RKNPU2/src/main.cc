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

#include "rknn_api.h"

// please set according to the your model
#define NUM_WORD 36808
#define BPE_LEN 30002
// #define NUM_WORD 23048
// #define BPE_LEN 9001

#define BPE_VERSION 0

#define HEAD_NUM 4
#define EMBEDDING_DIM 256
#define MAX_SENTENCE_LEN 16
#define DECODER_LAYER_NUM 3

#define LINE_MAX 512
#define MAX_USER_INPUT_LEN 64

/*-------------------------------------------
                  Functions
-------------------------------------------*/
void safe_flush()
    {
        char c;
        while((c = getchar()) != '\n' && c != EOF);
    }

static void printRKNNTensor(rknn_tensor_attr *attr)
{
    printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, 0, attr->type, attr->qnt_type, attr->fl, attr->zp, attr->scale);
}

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static int64_t getCurrentTimeUs()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}


float *encoder_embedding(float *token_embed, float *position_embed, int *input_word)
{
    static float embedding[MAX_SENTENCE_LEN * EMBEDDING_DIM];
    float scale = sqrt(EMBEDDING_DIM);
    int pad = 1;
    for (int i = 0; i < MAX_SENTENCE_LEN; i++)
    {
        for (int j = 0; j < EMBEDDING_DIM; j++)
        {
            embedding[i * EMBEDDING_DIM + j] = token_embed[input_word[i] * EMBEDDING_DIM + j] * scale;
        }
    }

    for (int i = 0; i < MAX_SENTENCE_LEN; i++)
    {
        if (input_word[i] != 1)
        {
            pad++;
        }
        else {
            pad = 1;
        }
        for (int j = 0; j < EMBEDDING_DIM; j++)
        {
            embedding[i * EMBEDDING_DIM + j] += position_embed[EMBEDDING_DIM * pad + j];
        }
    }
    return embedding;
}

float *decoder_embedding(float *token_embed, float *position_embed, int *input_word)
{
    static float embedding[MAX_SENTENCE_LEN * EMBEDDING_DIM];
    float scale = sqrt(EMBEDDING_DIM);
    int pad = 1;
    for (int i = 0; i < MAX_SENTENCE_LEN; i++)
    {
        for (int j = 0; j < EMBEDDING_DIM; j++)
        {
            embedding[i * EMBEDDING_DIM + j] = token_embed[input_word[i] * EMBEDDING_DIM + j] * scale;
        }
    }

    for (int i = 0; i < MAX_SENTENCE_LEN; i++)
    {
        if (input_word[i] != 1)
        {
            pad++;
        }
        else
        {
            pad = 1;
        }
        for (int j = 0; j < EMBEDDING_DIM; j++)
        {
            embedding[i * EMBEDDING_DIM + j] += position_embed[EMBEDDING_DIM * pad + j];
        }
    }
    return embedding;
}

int find_bpe_code(char bpe[][512], char key[])
{
    for (int i = 0; i < BPE_LEN; i++)
    {
        if (strcmp(bpe[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}

int find_dict(char dict[][512], char key[])
{
    for (int i = 0; i < NUM_WORD; i++)
    {
        if (strcmp(dict[i], key) == 0)
        {
            return i + 4;
        }
    }
    return 3;
}

// 1x4x16x64 -> 1x15x64x4
int preprocess_prev_key_value(float *prev_data, float *save_data) {
    float mid_data[MAX_SENTENCE_LEN*EMBEDDING_DIM];

    // 1x4x16x64->1x16x64x4
    for (int s=0; s<MAX_SENTENCE_LEN*EMBEDDING_DIM/HEAD_NUM; s++) {
        for (int h=0; h<HEAD_NUM; h++) {
            mid_data[s*HEAD_NUM+h] = save_data[h*MAX_SENTENCE_LEN*EMBEDDING_DIM/HEAD_NUM+s];
        }
    }

    // 1x16x64x4->1x15x64x4
    memcpy(prev_data, mid_data+EMBEDDING_DIM, (MAX_SENTENCE_LEN-1)*EMBEDDING_DIM*sizeof(float));

    return 0;
}

/*-------------------------------------------
                  Main Functions
-------------------------------------------*/
int main(int argc, char **argv)
{
    char *model_name = NULL;
    int ret1, ret2;
    int encoder_model_len = 0;
    int decoder_model_len = 0;
    rknn_context ctx1, ctx2;
    u_int64_t old_time;

    if (argc != 3)
    {
        printf("Usage: %s <rknn encoder model> <rknn decoder model> \n", argv[0]);
        return -1;
    }

    const char *encoder_model_path = argv[1];
    const char *decoder_model_path = argv[2];
    const char split[] = "</w>";

    FILE *fp_dict;
    int dict_line_num = 0;             // 文件行数
    static char dict[NUM_WORD][LINE_MAX]; // 行数据缓存
    static char dict_txt[] = "model/dict.txt";

    fp_dict = fopen(dict_txt, "r");
    if (NULL == fp_dict)
    {
        printf("open %s failed.\n", dict_txt);
        return -1;
    }

    while (fgets(dict[dict_line_num], LINE_MAX, fp_dict))
    {
        for (int j = 0; j < 512; j++)
        {
            if (dict[dict_line_num][j] == '\n')
            {
                dict[dict_line_num][j] = '\0';
            }
        }
        dict_line_num++;
    }
    fclose(fp_dict);

    FILE *fp_bpe;
    int bpe_line_num = 0;             // 文件行数
    static char bpe[BPE_LEN][512] = {}; // 行数据缓存
    static char bpe_txt[] = "model/bpe.txt";
    fp_bpe = fopen(bpe_txt, "r");

    if (NULL == fp_bpe)
    {
        printf("open %s failed.\n", bpe_txt);
        return -1;
    }

    while (fgets(bpe[bpe_line_num], LINE_MAX, fp_bpe))
    {
        for (int j = 0; j < 512; j++)
        {
            if (bpe[bpe_line_num][j] == '\n')
            {
                bpe[bpe_line_num][j] = '\0';
            }
        }
        /** 对每行数据(bpe)进行处理 **/
        bpe_line_num++;
    }

    fclose(fp_bpe);

    /* Create the neural network */
    printf("%s \n", encoder_model_path);
    printf("%s \n", decoder_model_path);

    ret1 = rknn_init(&ctx1, (void *)encoder_model_path, 0, 0, NULL);
    if (ret1 < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret1);
        return -1;
    }

    ret2 = rknn_init(&ctx2, (void *)decoder_model_path, 0, 0, NULL);
    if (ret2 < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret2);
        return -1;
    }

    /* Get Encoder Model Input Output Info */
    rknn_input_output_num encoder_io_num;
    ret1 = rknn_query(ctx1, RKNN_QUERY_IN_OUT_NUM, &encoder_io_num, sizeof(encoder_io_num));
    if (ret1 != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret1);
        return -1;
    }
    printf("encoder model input num: %d, output num: %d\n", encoder_io_num.n_input, encoder_io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr encoder_input_attrs[encoder_io_num.n_input];
    memset(encoder_input_attrs, 0, sizeof(encoder_input_attrs));
    for (int i = 0; i < encoder_io_num.n_input; i++)
    {
        encoder_input_attrs[i].index = i;
        ret1 = rknn_query(ctx1, RKNN_QUERY_INPUT_ATTR, &(encoder_input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret1 != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret1);
            return -1;
        }
        dump_tensor_attr(&(encoder_input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr encoder_output_attrs[encoder_io_num.n_output];
    memset(encoder_output_attrs, 0, sizeof(encoder_output_attrs));
    for (int i = 0; i < encoder_io_num.n_output; i++)
    {
        encoder_output_attrs[i].index = i;
        ret1 = rknn_query(ctx1, RKNN_QUERY_OUTPUT_ATTR, &(encoder_output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret1 != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret1);
            return -1;
        }
        dump_tensor_attr(&(encoder_output_attrs[i]));
    }

    printf("===================================\n");

    /* Get decoder Model Input Output Info */
    rknn_input_output_num decoder_io_num;
    ret2 = rknn_query(ctx2, RKNN_QUERY_IN_OUT_NUM, &decoder_io_num, sizeof(decoder_io_num));
    if (ret2 != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret2);
        return -1;
    }
    printf("decoder model input num: %d, output num: %d\n", decoder_io_num.n_input, decoder_io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr decoder_input_attrs[decoder_io_num.n_input];
    memset(decoder_input_attrs, 0, sizeof(decoder_input_attrs));
    for (int i = 0; i < decoder_io_num.n_input; i++)
    {
        decoder_input_attrs[i].index = i;
        ret2 = rknn_query(ctx2, RKNN_QUERY_INPUT_ATTR, &(decoder_input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret2 != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret2);
            return -1;
        }
        dump_tensor_attr(&(decoder_input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr decoder_output_attrs[decoder_io_num.n_output];
    memset(decoder_output_attrs, 0, sizeof(decoder_output_attrs));
    for (int i = 0; i < decoder_io_num.n_output; i++)
    {
        decoder_output_attrs[i].index = i;
        ret2 = rknn_query(ctx2, RKNN_QUERY_OUTPUT_ATTR, &(decoder_output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret2 != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret2);
            return -1;
        }
        dump_tensor_attr(&(decoder_output_attrs[i]));
    }

    printf("===================================\n");

    // Set Encoder Input Data
    static float token_embed[NUM_WORD * EMBEDDING_DIM];
    char * token_embed_file = "model/token_embed.bin";
    FILE *fp_token_embed = fopen(token_embed_file, "rb");

    if (fp_token_embed != NULL) {
        fread(token_embed, sizeof(float), NUM_WORD * EMBEDDING_DIM, fp_token_embed);
        fclose(fp_token_embed);
    } else {
        printf("Open %s fail!\n", token_embed_file);
        return -1;   
    }

    static float position_embed[1026 * EMBEDDING_DIM];
    char * position_embed_file = "model/position_embed.bin";
    FILE *fp_position_embed = fopen(position_embed_file, "rb");

    if (fp_position_embed != NULL) {
        fread(position_embed, sizeof(float), 1026 * EMBEDDING_DIM, fp_position_embed);
        fclose(fp_position_embed);
    } else {
        printf("Open %s fail!\n", position_embed_file);
        return -1;   
    }

    char input_sentence[MAX_USER_INPUT_LEN];
    char input_word[MAX_USER_INPUT_LEN][MAX_USER_INPUT_LEN];
    while (1)
    {
        memset(input_sentence, 0, MAX_USER_INPUT_LEN);
        memset(input_word, 0, MAX_USER_INPUT_LEN*MAX_USER_INPUT_LEN);
        // rewind(stdin);
        printf("请输入需要翻译的英文,输入q退出:\n");
        scanf("%[^\n]", input_sentence);
        safe_flush();

        printf("===================================\n");

        old_time = getCurrentTimeUs();

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
            input_word[num_word][c] = input_sentence[i];
            c++;
        }

        if (num_word==0 && strcmp(input_word[0], "q")==0){
            break;
        }

        int num_char = 1;
        int tokens[MAX_SENTENCE_LEN];
        int token_len = 0;
        for (int i = 0; i <= num_word; i++)
        {
            num_char = strlen(input_word[i]);
            if (num_char > 1)
            {
                char sub_word[num_char][54];
                for (int j = 0; j < num_char; j++)
                {
                    sprintf(sub_word[j], "%c", input_word[i][j]);
                }

#if (BPE_VERSION == 1)
                sprintf(sub_word[num_char], split);         // add split symbol
                num_char += 1;
#elif (BPE_VERSION == 0)
                sprintf(sub_word[num_char-1], "%s%s", sub_word[num_char-1], split);         // add split symbol
#endif

                while (num_char > 1)
                {
                    char key[54] = {};
                    int score;
                    int cat_index = 0;
                    int best_score = 12345678;
                    for (int j = 0; j < num_char - 1; j++)
                    {
                        sprintf(key, "%s %s", sub_word[j], sub_word[j + 1]);
                        score = find_bpe_code(bpe, key);
                        if (best_score > score and score >= 0) // score越小越好
                        {
                            best_score = score;
                            cat_index = j;
                        }
                    }

                    if (best_score == 12345678)
                    {
                        break;
                    }

                    num_char--;
                    sprintf(sub_word[cat_index], "%s%s", sub_word[cat_index], sub_word[cat_index + 1]);
                    for (int j = cat_index + 1; j < num_char; j++)
                    {
                        sprintf(sub_word[j], "%s", sub_word[j + 1]);
                    }

                    // printf("    sub_word[%d]: %s\n", cat_index, sub_word[cat_index]);
                    // printf("    full word:");
                    // for (int j=0; j<num_char; j++){
                    //     printf(" %s", sub_word[j]);
                    // }
                    // printf("\n");
                    // printf("    char: %d\n", num_char);
                }

                //删除split符号
                if (strcmp(sub_word[num_char - 1], split) == 0){
                    num_char -= 1;
                }
                else{
                    int w = strlen(sub_word[num_char - 1]);
                    for (int j = w - 4; j < w; j++) //删除split符号
                    {
                        sub_word[num_char - 1][j] = '\0';
                    }
                }

                if (num_char > 1)
                {
                    for (int j = 0; j < num_char - 1; j++) //添加连接符号
                    {
                        sprintf(sub_word[j], "%s%s", sub_word[j], "@@");
                    }
                }

                printf("input_word: %s\n", input_word[i]);
                for (int j = 0; j < num_char; j++)
                {
                    tokens[token_len] = find_dict(dict, sub_word[j]);
                    if (tokens[token_len]==3){
                        printf("    bpe as: %s (not found in dict, set as <unk>)\n", sub_word[j]);
                    }
                    else{
                        printf("    bpe as: %s\n", sub_word[j]);
                    }
                    token_len++;
                }
            }
            else
            {
                printf("input_word: %s\n    bpe as: %s\n", input_word[i], input_word[i]);
                tokens[token_len] = find_dict(dict, input_word[i]);
                token_len++;
            }
        }

        int enc_input_word[MAX_SENTENCE_LEN] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2};
        for (int i = 0; i < token_len; i++)
        {
            enc_input_word[MAX_SENTENCE_LEN - token_len - 1 + i] = tokens[i];
        }

        // fix input for debug
        // int sp_token[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   1,   1, 912, 2431, 2};           // hello

        // for debug
        printf("enc input tokens:\n");
        for (int i = 0; i < MAX_SENTENCE_LEN; i++)
        {
            // enc_input_word[i] = sp_token[i];
            printf("%d ", enc_input_word[i]);
        }
        printf("\n");

        int enc_pad_num = 0;
        for (int i = 0; i < MAX_SENTENCE_LEN; i++)
        {
            if (enc_input_word[i] == 1)
            {
                enc_pad_num++;
            }
        }

        float *embedding = encoder_embedding(token_embed, position_embed, enc_input_word);
        rknn_input encoder_inputs[encoder_io_num.n_input];
        memset(encoder_inputs, 0, sizeof(encoder_inputs));

        int enc_input_size0 = MAX_SENTENCE_LEN;
        int enc_input_size1 = EMBEDDING_DIM;
        encoder_inputs[0].index = 0;
        encoder_inputs[0].type = RKNN_TENSOR_FLOAT32;
        encoder_inputs[0].size = enc_input_size0 * enc_input_size1 * 4;
        encoder_inputs[0].fmt = RKNN_TENSOR_UNDEFINED;
        encoder_inputs[0].buf = embedding;

        int enc_pad_mask_size0 = MAX_SENTENCE_LEN;
        int enc_pad_mask_size1 = MAX_SENTENCE_LEN;

        float enc_pad_mask[MAX_SENTENCE_LEN * MAX_SENTENCE_LEN];
        float dec_enc_pad_mask[MAX_SENTENCE_LEN];
        float pad = 1;

        for (int i = 0; i < MAX_SENTENCE_LEN; i++)
        {
            if (i < enc_pad_num)
            {
                pad = 1;
            }
            else
            {
                pad = 0;
            }
            for (int j = 0; j < MAX_SENTENCE_LEN; j++)
            {
                enc_pad_mask[j * MAX_SENTENCE_LEN + i] = pad;
            }
            dec_enc_pad_mask[i] = pad;
        }

        encoder_inputs[1].index = 1;
        encoder_inputs[1].type = RKNN_TENSOR_FLOAT32;
        encoder_inputs[1].size = enc_pad_mask_size0 * enc_pad_mask_size1 * 4;
        encoder_inputs[1].fmt = RKNN_TENSOR_UNDEFINED;
        encoder_inputs[1].buf = enc_pad_mask;

        printf("Encoder preprocess use time: %.2f ms\n", (getCurrentTimeUs()-old_time)/1000.f);
        old_time = getCurrentTimeUs();

        ret1 = rknn_inputs_set(ctx1, encoder_io_num.n_input, encoder_inputs);
        if (ret1 < 0)
        {
            printf("rknn_input_set fail! ret=%d\n", ret1);
            return -1;
        }

        // Run
        printf("rknn encoder run\n");
        ret1 = rknn_run(ctx1, nullptr);
        if (ret1 < 0)
        {
            printf("rknn_run fail! ret=%d\n", ret1);
            return -1;
        }

        // Get Output
        rknn_output encoder_outputs[encoder_io_num.n_output];
        memset(encoder_outputs, 0, sizeof(encoder_outputs));
        encoder_outputs[0].want_float = 1;
        ret1 = rknn_outputs_get(ctx1, encoder_io_num.n_output, encoder_outputs, NULL);
        if (ret1 < 0)
        {
            printf("rknn_encoder_outputs_get fail! ret=%d\n", ret1);
            return -1;
        }
        printf("Encoder inference use time: %.2f ms\n", (getCurrentTimeUs()-old_time)/1000.f);

        // Set Decoder Input Data
        rknn_input decoder_inputs[decoder_io_num.n_input];
        memset(decoder_inputs, 0, sizeof(decoder_inputs));
        rknn_output decoder_outputs[decoder_io_num.n_output];
        memset(decoder_outputs, 0, sizeof(decoder_outputs));

        float prev_key[DECODER_LAYER_NUM][(MAX_SENTENCE_LEN-1) * EMBEDDING_DIM];
        float prev_value[DECODER_LAYER_NUM][(MAX_SENTENCE_LEN-1) * EMBEDDING_DIM];

        memset(prev_key, 0x00, sizeof(prev_key));
        memset(prev_value, 0x00, sizeof(prev_value));

        int dec_input_word[MAX_SENTENCE_LEN] = {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        
        printf("rknn decoder run\n");
        old_time = getCurrentTimeUs();

        for (int num_iter = 0; num_iter < MAX_SENTENCE_LEN; num_iter++)
        {
            float *dec_input = decoder_embedding(token_embed, position_embed, dec_input_word);
            int dec_input_size0 = MAX_SENTENCE_LEN;
            int dec_input_size1 = EMBEDDING_DIM;
            decoder_inputs[0].index = 0;
            decoder_inputs[0].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[0].size = dec_input_size1 * 4;
            decoder_inputs[0].fmt = RKNN_TENSOR_UNDEFINED;
            decoder_inputs[0].buf = dec_input + num_iter*dec_input_size1;

            int enc_outut_size0 = MAX_SENTENCE_LEN;
            int enc_outut_size1 = EMBEDDING_DIM;
            decoder_inputs[1].index = 1;
            decoder_inputs[1].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[1].size = enc_outut_size0 * enc_outut_size1 * 4;
            decoder_inputs[1].fmt = RKNN_TENSOR_UNDEFINED;
            decoder_inputs[1].buf = encoder_outputs[0].buf;

            decoder_inputs[2].index = 2;
            decoder_inputs[2].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[2].size = enc_pad_mask_size1 * 4;
            decoder_inputs[2].fmt = RKNN_TENSOR_UNDEFINED;
            decoder_inputs[2].buf = dec_enc_pad_mask;

            float dec_mask[ MAX_SENTENCE_LEN];
            float mask;

            for (int j = 0; j < MAX_SENTENCE_LEN; j++)
            {
                if (j >= MAX_SENTENCE_LEN - 1 - num_iter)
                {
                    mask = 0;
                }
                else
                {
                    mask = 1;
                }
                dec_mask[j] = mask;
            }

            decoder_inputs[3].index = 3;
            decoder_inputs[3].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[3].size = sizeof(dec_mask);
            decoder_inputs[3].fmt = RKNN_TENSOR_UNDEFINED;
            decoder_inputs[3].buf = dec_mask;

            // incremental inference
            if (num_iter != 0) {
                preprocess_prev_key_value(prev_key[0], (float *)decoder_outputs[1].buf);
                preprocess_prev_key_value(prev_value[0], (float *)decoder_outputs[2].buf);
                preprocess_prev_key_value(prev_key[1], (float *)decoder_outputs[3].buf);
                preprocess_prev_key_value(prev_value[1], (float *)decoder_outputs[4].buf);
                preprocess_prev_key_value(prev_key[2], (float *)decoder_outputs[5].buf);
                preprocess_prev_key_value(prev_value[2], (float *)decoder_outputs[6].buf);

                rknn_outputs_release(ctx2, decoder_io_num.n_output, decoder_outputs);
            }

            decoder_inputs[4].index = 4;
            decoder_inputs[4].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[4].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[4].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[4].buf = prev_key[0];

            decoder_inputs[5].index = 5;
            decoder_inputs[5].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[5].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[5].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[5].buf = prev_value[0];

            decoder_inputs[6].index = 6;
            decoder_inputs[6].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[6].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[6].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[6].buf = prev_key[1];

            decoder_inputs[7].index = 7;
            decoder_inputs[7].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[7].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[7].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[7].buf = prev_value[1];

            decoder_inputs[8].index = 8;
            decoder_inputs[8].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[8].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[8].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[8].buf = prev_key[2];

            decoder_inputs[9].index = 9;
            decoder_inputs[9].type = RKNN_TENSOR_FLOAT32;
            decoder_inputs[9].size = (MAX_SENTENCE_LEN-1) * EMBEDDING_DIM * 4;
            decoder_inputs[9].fmt = RKNN_TENSOR_NHWC;
            decoder_inputs[9].buf = prev_value[2];

            ret2 = rknn_inputs_set(ctx2, decoder_io_num.n_input, decoder_inputs);
            if (ret2 < 0)
            {
                printf("rknn_input_set fail! ret=%d\n", ret2);
                return -1;
            }

            // Run
            ret2 = rknn_run(ctx2, nullptr);
            if (ret2 < 0)
            {
                printf("rknn_run fail! ret=%d\n", ret2);
                return -1;
            }

            // Get Output
            for (int j=0; j<decoder_io_num.n_output; j++) {
                decoder_outputs[j].want_float = 1;
            }
            ret2 = rknn_outputs_get(ctx2, decoder_io_num.n_output, decoder_outputs, NULL);
            if (ret2 < 0)
            {
                printf("rknn_decoder_outputs_get fail! ret=%d\n", ret2);
                return -1;
            }

            // argmax
            int max = 0;
            float value = ((float *)(decoder_outputs[0].buf))[0];
            for (int j = 1; j < NUM_WORD; j++)
            {
                if (((float *)(decoder_outputs[0].buf))[j] > value)
                {
                    value = ((float *)(decoder_outputs[0].buf))[j];
                    max = j;
                }
            }

            dec_input_word[num_iter + 1] = max;
            printf("%d : %f\n", max, value);            // for debug

            if (max == 2)
            {
                break;
            }
        }

        // Release rknn_outputs
        rknn_outputs_release(ctx2, decoder_io_num.n_output, decoder_outputs);

        // Release rknn_outputs
        rknn_outputs_release(ctx1, encoder_io_num.n_output, encoder_outputs);

        printf("Decoder use total time: %.2f ms\n", (getCurrentTimeUs()-old_time)/1000.f);

        // for debug
        for (int i = 0; i < MAX_SENTENCE_LEN; i++)
        {
            printf("%d ", dec_input_word[i]);
        }
        printf("\n");

        printf("===================================\n");
        char predict_word[512];
        for (int i = 1; i < MAX_SENTENCE_LEN; i++)
        {
            memset(predict_word, 0x00, sizeof(predict_word));
            if (dec_input_word[i] == 2)
            {
                break;
            }
            for (int j = 0; j < 512; j++)
            {
                if (dict[dec_input_word[i] - 4][j] == '@' and dict[dec_input_word[i] - 4][j + 1] == '@')
                {
                    break;
                }
                predict_word[j] = dict[dec_input_word[i] - 4][j];
            }
            printf("%s", predict_word);
        }
        printf("\n");
        printf("===================================\n");
    }
    


    // Release
    if (ctx1 > 0)
    {
        rknn_destroy(ctx1);
    }
    if (ctx2 > 0)
    {
        rknn_destroy(ctx2);
    }

    return 0;
}
