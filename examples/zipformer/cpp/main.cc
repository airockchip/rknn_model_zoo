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
#include "zipformer.h"
#include "audio_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include "process.h"
#include <iomanip>

/*-------------------------------------------
                  Main Function
-------------------------------------------*/

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        printf("%s <encoder_path> <decoder_path> <joiner_path> <audio_path>\n", argv[0]);
        return -1;
    }

    const char *encoder_path = argv[1];
    const char *decoder_path = argv[2];
    const char *joiner_path = argv[3];
    const char *audio_path = argv[4];

    int ret;
    TIMER timer;
    float infer_time = 0.0;
    float audio_length = 0.0;
    float rtf = 0.0;
    int frame_shift_ms = 10;
    int subsampling_factor = 4;
    float frame_shift_s = frame_shift_ms / 1000.0 * subsampling_factor;
    std::vector<std::string> recognized_text;
    std::vector<float> timestamp;
    rknn_zipformer_context_t rknn_app_ctx;
    VocabEntry vocab[VOCAB_NUM];
    audio_buffer_t audio;
    memset(&rknn_app_ctx, 0, sizeof(rknn_zipformer_context_t));
    memset(vocab, 0, sizeof(vocab));
    memset(&audio, 0, sizeof(audio_buffer_t));

    timer.tik();
    ret = read_audio(audio_path, &audio);
    if (ret != 0)
    {
        printf("read audio fail! ret=%d audio_path=%s\n", ret, audio_path);
        goto out;
    }

    if (audio.num_channels == 2)
    {
        ret = convert_channels(&audio);
        if (ret != 0)
        {
            printf("convert channels fail! ret=%d\n", ret, audio_path);
            goto out;
        }
    }

    if (audio.sample_rate != SAMPLE_RATE)
    {
        ret = resample_audio(&audio, audio.sample_rate, SAMPLE_RATE);
        if (ret != 0)
        {
            printf("resample audio fail! ret=%d\n", ret, audio_path);
            goto out;
        }
    }

    ret = read_vocab(VOCAB_PATH, vocab);
    if (ret != 0)
    {
        printf("read vocab fail! ret=%d vocab_path=%s\n", ret, VOCAB_PATH);
        goto out;
    }
    timer.tok();
    timer.print_time("read_audio & convert_channels & resample_audio & read_vocab");

    timer.tik();
    ret = init_zipformer_model(encoder_path, &rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("init_zipformer_model fail! ret=%d encoder_path=%s\n", ret, encoder_path);
        goto out;
    }
    build_input_output(&rknn_app_ctx.encoder_context);
    timer.tok();
    timer.print_time("init_zipformer_encoder_model");

    timer.tik();
    ret = init_zipformer_model(decoder_path, &rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("init_zipformer_model fail! ret=%d decoder_path=%s\n", ret, decoder_path);
        goto out;
    }
    build_input_output(&rknn_app_ctx.decoder_context);
    timer.tok();
    timer.print_time("init_zipformer_decoder_model");

    timer.tik();
    ret = init_zipformer_model(joiner_path, &rknn_app_ctx.joiner_context);
    if (ret != 0)
    {
        printf("init_zipformer_model fail! ret=%d oiner_path=%s\n", ret, joiner_path);
        goto out;
    }
    build_input_output(&rknn_app_ctx.joiner_context);
    timer.tok();
    timer.print_time("init_zipformer_joiner_model");

    timer.tik();
    ret = inference_zipformer_model(&rknn_app_ctx, audio, vocab, recognized_text, timestamp, audio_length);
    if (ret != 0)
    {
        printf("inference_zipformer_model fail! ret=%d\n", ret);
        goto out;
    }
    timer.tok();
    timer.print_time("inference_zipformer_model");

    infer_time = timer.get_time() / 1000.0; // sec
    rtf = infer_time / audio_length;
    printf("\nReal Time Factor (RTF): %.3f / %.3f = %.3f\n", infer_time, audio_length, rtf);

    // print result
    std::cout << "\nTimestamp (s): ";
    std::cout << std::fixed << std::setprecision(2);
    for (size_t i = 0; i < timestamp.size(); ++i)
    {
        std::cout << timestamp[i] * frame_shift_s;
        if (i < timestamp.size() - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;

    std::cout << "\nZipformer output: ";
    for (const auto &str : recognized_text)
    {
        std::cout << str;
    }
    std::cout << std::endl;

out:

    if (audio.data)
    {
        free(audio.data);
    }

    for (int i = 0; i < VOCAB_NUM; i++)
    {
        if (vocab[i].token)
        {
            free(vocab[i].token);
            vocab[i].token = NULL;
        }
    }

    ret = release_zipformer_model(&rknn_app_ctx.encoder_context);
    if (ret != 0)
    {
        printf("release_zipformer_model encoder_context fail! ret=%d\n", ret);
    }

    ret = release_zipformer_model(&rknn_app_ctx.decoder_context);
    if (ret != 0)
    {
        printf("release_zipformer_model decoder_context fail! ret=%d\n", ret);
    }

    ret = release_zipformer_model(&rknn_app_ctx.joiner_context);
    if (ret != 0)
    {
        printf("release_zipformer_model joiner_context fail! ret=%d\n", ret);
    }

    return 0;
}
