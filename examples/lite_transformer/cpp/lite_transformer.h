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


#ifndef _RKNN_DEMO_LITE_TRANSFORMER_H_
#define _RKNN_DEMO_LITE_TRANSFORMER_H_

#include "rknn_api.h"
#include "common.h"
#include "rknn_demo_utils.h"
#include "easy_timer.h"
#include "bpe_tools.h"

#define HEAD_NUM 4
#define EMBEDDING_DIM 256
#define DECODER_LAYER_NUM 3
#define MAX_SENTENCE_LEN 16

// #define DECODER_LAYER_NUM 6

#define POS_LEN 1026

#define ENCODER_INPUT_TOKEN_RIGHTSIDE_ALIGN

#define MAX_WORD_NUM_IN_SENTENCE 64
#define MAX_WORD_LEN 64

#define MAX_USER_INPUT_LEN 1024


typedef struct _NMT_TOKENS{
    float *enc_token_embed;
    float *enc_pos_embed;
    float *dec_token_embed;
    float *dec_pos_embed;
} NMT_TOKENS;

typedef struct {
    MODEL_INFO enc;
    MODEL_INFO dec;
    NMT_TOKENS nmt_tokens;
    Bpe_Tools *bpe_tools;

    int enc_len;
    int dec_len;
} rknn_lite_transformer_context_t;


int init_lite_transformer_model(const char* encoder_path, 
                                const char* decoder_path,
                                const char* token_embed_path,
                                const char* pos_embed_path,
                                const char* bpe_dict_path,
                                const char* token_dict_path,
                                const char* common_word_path,
                                DICT_ORDER_TYPE dict_order_type,
                                rknn_lite_transformer_context_t* app_ctx);

int release_lite_transformer_model(rknn_lite_transformer_context_t* app_ctx);

int inference_lite_transformer_model(rknn_lite_transformer_context_t* app_ctx, 
                                     const char* input_sentence, 
                                     char* output_sentence);

#endif //_RKNN_DEMO_LITE_TRANSFORMER_H_