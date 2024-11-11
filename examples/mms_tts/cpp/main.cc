// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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
#include "mms_tts.h"
#include <iostream>
#include <vector>
#include <string>

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("%s <encoder_path> <decoder_path> <input_text>\n", argv[0]);
        return -1;
    }

    const char *encoder_path = argv[1];
    const char *decoder_path = argv[2];
    const char *input_text = argv[3];
    const char *audio_save_path = "output.wav";

    int ret;
    TIMER timer;
    rknn_mms_tts_context_t rknn_app_ctx;
    std::map<char, int> vocab;
    std::vector<int64_t> input_ids(MAX_LENGTH, 0);
    std::vector<int64_t> attention_mask(MAX_LENGTH, 0);
    float infer_time = 0.0;
    int predicted_lengths_max_real = 0;
    float audio_length = 0.0;
    float max_audio_length = (float)PREDICTED_LENGTHS_MAX * PREDICTED_BATCH / SAMPLE_RATE;
    float rtf = 0.0;
    memset(&rknn_app_ctx, 0, sizeof(rknn_mms_tts_context_t));

    timer.tik();
    ret = init_mms_tts_model(encoder_path, &rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("init_mms_tts_model fail! ret=%d encoder_path=%s\n", ret, encoder_path);
        goto out;
    }
    timer.tok();
    timer.print_time("init_mms_tts_encoder_model");

    timer.tik();
    ret = init_mms_tts_model(decoder_path, &rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("init_mms_tts_model fail! ret=%d decoder_path=%s\n", ret, decoder_path);
        goto out;
    }
    timer.tok();
    timer.print_time("init_mms_tts_decoder_model");

    // set data
    timer.tik();
    read_vocab(vocab);
    timer.tok();
    timer.print_time("read_vocab");

    timer.tik();
    preprocess_input(input_text, vocab, VOCAB_NUM, MAX_LENGTH, input_ids, attention_mask);
    ret = inference_mms_tts_model(&rknn_app_ctx, input_ids, attention_mask, predicted_lengths_max_real, audio_save_path);
    if (ret != 0)
    {
        printf("inference_mms_tts_model fail! ret=%d\n", ret);
        goto out;
    }
    timer.tok();
    timer.print_time("inference_mms_tts_model");

    infer_time = timer.get_time() / 1000.0; // sec
    audio_length = max_audio_length;        // sec
    rtf = infer_time / audio_length;
    printf("\nReal Time Factor (RTF): %.3f / %.3f = %.3f\n", infer_time, audio_length, rtf);
    printf("\nThe output wav file is saved: %s\n", audio_save_path);

out:

    ret = release_mms_tts_model(&rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("release_mms_tts_model encoder_context fail! ret=%d\n", ret);
    }
    ret = release_mms_tts_model(&rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("release_ppocr_model decoder_context fail! ret=%d\n", ret);
    }

    return 0;
}
