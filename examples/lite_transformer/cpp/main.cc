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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lite_transformer.h"
#include "file_utils.h"
#include "rknn_demo_utils.h"
#include "bpe_tools.h"
#include "easy_timer.h"


void safe_flush()
{
    char c;
    while((c = getchar()) != '\n' && c != EOF);
}

int read_user_input(char* type_in_sentence){
    rewind(stdin);
    printf("请输入需要翻译的英文,输入q退出:\n");
    scanf("%[^\n]", type_in_sentence);
    safe_flush();

    if (strcmp(type_in_sentence, "q") == 0){
        return -1;
    }
    return 0;
}


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("%s <encoder_path> <decoder_path> <sentence>\n", argv[0]);
        return -1;
    }

    TIMER timer;

    const char *encoder_path = argv[1];
    const char *decoder_path = argv[2];

    const char* token_embed_path = "./model/token_embed.bin";
    const char* pos_embed_path = "./model/position_embed.bin";

    DICT_ORDER_TYPE dict_order_type = ORDERED;
    const char *bpe_dict_path = "./model/bpe_order.txt";
    const char *token_dict_path = "./model/dict_order.txt";
    const char *common_word_path = "model/cw_token_map_order.txt";

    // DICT_ORDER_TYPE dict_order_type = NON_ORDERED;
    // const char *bpe_dict_path = "model/bpe.txt";
    // const char *token_dict_path = "model/dict.txt";
    // const char *common_word_path = nullptr;

    int ret;
    rknn_lite_transformer_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_lite_transformer_context_t));

    char *input_strings = (char *)malloc(MAX_USER_INPUT_LEN);
    char *output_strings = (char *)malloc(MAX_USER_INPUT_LEN);
    memset(input_strings, 0, MAX_USER_INPUT_LEN);
    memset(output_strings, 0, MAX_USER_INPUT_LEN);

    bool is_receipt = false;

    ret = init_lite_transformer_model(encoder_path, 
                                      decoder_path,
                                      token_embed_path,
                                      pos_embed_path,
                                      bpe_dict_path, 
                                      token_dict_path, 
                                      common_word_path,
                                      dict_order_type,
                                      &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_lite_transformer_model fail!\n");
        goto out;
    }

    // receipt string to translate
    if (argc > 3)
    {
        is_receipt = true;
        for (int i = 3; i < argc; i++)
        {
            strcat(input_strings, argv[i]);
            strcat(input_strings, " ");
        }
    }

    while (1)
    {
        if (is_receipt == false)
        {
            memset(input_strings, 0, MAX_USER_INPUT_LEN);
            int num_word = read_user_input(input_strings);
            if (num_word == -1)
            {
                break;
            }
        }

        timer.tik();
        ret = inference_lite_transformer_model(&rknn_app_ctx, input_strings, output_strings);
        if (ret != 0)
        {
            printf("lite_transformer_model inference fail! ret=%d\n", ret);
            break;
        }
        timer.tok();
        timer.print_time("inference time");

        printf("output_strings: %s\n", output_strings);

        if (is_receipt == true)
        {
            break;
        }
    }

out:
    ret = release_lite_transformer_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_lite_transformer_model fail! ret=%d\n", ret);
    }
    free(input_strings);
    free(output_strings);

    return 0;
}
