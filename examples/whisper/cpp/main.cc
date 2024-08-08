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
#include "whisper.h"
#include "audio_utils.h"
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
        printf("%s <encoder_path> <decoder_path> <audio_path>\n", argv[0]);
        return -1;
    }

    const char *encoder_path = argv[1];
    const char *decoder_path = argv[2];
    const char *audio_path = argv[3];

    int ret;
    TIMER timer;
    float infer_time = 0.0;
    float audio_length = 0.0;
    float rtf = 0.0;
    rknn_whisper_context_t rknn_app_ctx;
    std::vector<std::string> recognized_text;
    float *mel_filters = (float *)malloc(N_MELS * MELS_FILTERS_SIZE * sizeof(float));
    VocabEntry vocab[VOCAB_NUM];
    audio_buffer_t audio;

    memset(&rknn_app_ctx, 0, sizeof(rknn_whisper_context_t));
    memset(vocab, 0, sizeof(vocab));
    memset(&audio, 0, sizeof(audio_buffer_t));

    timer.tik();
    ret = init_whisper_model(encoder_path, &rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("init_whisper_model fail! ret=%d encoder_path=%s\n", ret, encoder_path);
        return -1;
    }
    timer.tok();
    timer.print_time("init_whisper_encoder_model");

    timer.tik();
    ret = init_whisper_model(decoder_path, &rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("init_whisper_model fail! ret=%d decoder_path=%s\n", ret, decoder_path);
        return -1;
    }
    timer.tok();
    timer.print_time("init_whisper_decoder_model");

    // set data
    timer.tik();
    ret = read_mel_filters(MEL_FILTERS_PATH, mel_filters, N_MELS * MELS_FILTERS_SIZE);
    if (ret != 0)
    {
        printf("read mel_filters fail! ret=%d mel_filters_path=%s\n", ret, MEL_FILTERS_PATH);
        goto out;
    }

    ret = read_vocab(VOCAB_PATH, vocab);
    if (ret != 0)
    {
        printf("read vocab fail! ret=%d vocab_path=%s\n", ret, VOCAB_PATH);
        goto out;
    }

    ret = read_audio(audio_path, &audio);
    if (ret != 0)
    {
        printf("read audio fail! ret=%d audio_path=%s\n", ret, audio_path);
        goto out;
    }
    timer.tok();
    timer.print_time("read_mel_filters & read_vocab & read_audio ");

    timer.tik();
    ret = inference_whisper_model(&rknn_app_ctx, &audio, mel_filters, vocab, recognized_text);
    if (ret != 0)
    {
        printf("inference_whisper_model fail! ret=%d\n", ret);
        goto out;
    }
    timer.tok();
    timer.print_time("inference_whisper_model");

    // print result
    std::cout << "\nWhisper result:";
    for (const auto &str : recognized_text)
    {
        std::cout << str;
    }
    std::cout << std::endl;

    infer_time = timer.get_time() / 1000.0;               // sec
    audio_length = audio.num_frames / (float)SAMPLE_RATE; // sec
    audio_length = audio_length > (float)CHUNK_LENGTH ? (float)CHUNK_LENGTH : audio_length;
    rtf = infer_time / audio_length;
    printf("\nReal Time Factor (RTF): %.3f / %.3f = %.3f\n", infer_time, audio_length, rtf);

out:

    ret = release_whisper_model(&rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("release_whisper_model encoder_context fail! ret=%d\n", ret);
    }
    ret = release_whisper_model(&rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("release_ppocr_model decoder_context fail! ret=%d\n", ret);
    }

    if (audio.data != NULL)
    {
        free(audio.data);
    }

    for (int i = 0; i < VOCAB_NUM; ++i)
    {
        if (vocab[i].token != NULL)
        {
            free(vocab[i].token);
            vocab[i].token = NULL;
        }
    }

    if (mel_filters != NULL)
    {
        free(mel_filters);
    }

    return 0;
}
