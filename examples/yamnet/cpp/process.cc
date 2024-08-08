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

#include "yamnet.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pad_or_trim(const std::vector<float> &array, std::vector<float> &result, int array_shape, int length)
{
    if (array_shape > length)
    {
        std::copy(array.begin(), array.begin() + length, result.begin());
    }
    else
    {
        std::copy(array.begin(), array.end(), result.begin());
        std::fill(result.begin() + array_shape, result.end(), 0.0f);
    }
}

static int argmax(float array[], int size)
{
    int max_index = 0;
    float max_value = array[0];

    for (int i = 1; i < size; i++)
    {
        if (array[i] > max_value)
        {
            max_index = i;
            max_value = array[i];
        }
    }

    return max_index;
}

int read_label(LabelEntry *label)
{
    FILE *fp;
    char line[256];

    fp = fopen(LABEL_PATH, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    int count = 0;
    while (fgets(line, sizeof(line), fp))
    {
        label[count].token = strdup(strchr(line, ' ') + 1); // Get token after the first space
        label[count].index = atoi(line);                    // Get index before the first space
        count++;
    }

    fclose(fp);

    return 0;
}

int audio_preprocess(audio_buffer_t *audio, float *audio_pad_or_trim)
{
    std::vector<float> ori_audio_data(audio->data, audio->data + audio->num_frames);
    std::vector<float> audio_data(N_SAMPLES);

    pad_or_trim(ori_audio_data, audio_data, audio->num_frames, N_SAMPLES);
    memcpy(audio_pad_or_trim, audio_data.data(), N_SAMPLES * sizeof(float));
    return 0;
}

int post_process(float *scores, LabelEntry *label, ResultEntry *result)
{
    int num_rows = N_ROWS;
    int num_columns = LABEL_NUM;

    float mean_scores[LABEL_NUM] = {0};

    for (int j = 0; j < num_columns; j++)
    {
        float sum = 0;
        for (int i = 0; i < num_rows; i++)
        {
            sum += scores[i * num_columns + j];
        }
        mean_scores[j] = sum / num_rows;
    }

    int top_class_index = argmax(mean_scores, num_columns);

    result[0].index = top_class_index;
    result[0].token = label[top_class_index].token;

    return 0;
}