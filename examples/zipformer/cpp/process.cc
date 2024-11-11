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

#include "zipformer.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void convert_nchw_to_nhwc(float *src, float *dst, int N, int channels, int height, int width)
{
    for (int n = 0; n < N; ++n)
    {
        for (int c = 0; c < channels; ++c)
        {
            for (int h = 0; h < height; ++h)
            {
                for (int w = 0; w < width; ++w)
                {
                    dst[n * height * width * channels + h * width * channels + w * channels + c] = src[n * channels * height * width + c * height * width + h * width + w];
                }
            }
        }
    }
}

int get_kbank_frames(knf::OnlineFbank *fbank, int frame_index, int segment, float *frames)
{
    if (frame_index + segment > fbank->NumFramesReady())
    {
        return -1;
    }

    for (int i = 0; i < segment; ++i)
    {
        const float *frame = fbank->GetFrame(i + frame_index);
        memcpy(frames + i * N_MELS, frame, N_MELS * sizeof(float));
    }

    return 0;
}

int argmax(float *array)
{
    int start_index = 0;
    int max_index = start_index;
    float max_value = array[max_index];
    for (int i = start_index + 1; i < start_index + JOINER_OUTPUT_SIZE; i++)
    {
        if (array[i] > max_value)
        {
            max_value = array[i];
            max_index = i;
        }
    }
    int relative_index = max_index - start_index;
    return relative_index;
}

void replace_substr(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return; // Prevent infinite loop if 'from' is empty
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Advance position by length of the replacement
    }
}

int read_vocab(const char *fileName, VocabEntry *vocab)
{
    FILE *fp;
    char line[512];

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    int count = 0;
    while (fgets(line, sizeof(line), fp))
    {
        vocab[count].index = atoi(strchr(line, ' ') + 1); // get token before the first space
        char *token = strtok(line, " ");
        vocab[count].token = strdup(token); // Get index after the first space

        count++;
    }

    fclose(fp);

    return 0;
}