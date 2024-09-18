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

#include "wav2vec2.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TokenDictEntry tokenizer_dict[] = {
            {0, "<pad>"}, {1, "<s>"}, {2, "</s>"}, {3, "<unk>"}, {4, "|"}, {5, "E"}, {6, "T"}, {7, "A"}, 
            {8, "O"}, {9, "N"}, {10, "I"}, {11, "H"}, {12, "S"}, {13, "R"}, {14, "D"}, {15, "L"}, 
            {16, "U"}, {17, "M"}, {18, "W"}, {19, "C"}, {20, "F"}, {21, "G"}, {22, "Y"}, {23, "P"}, 
            {24, "B"}, {25, "V"}, {26, "K"}, {27, "'"}, {28, "X"}, {29, "J"}, {30, "Q"}, {31, "Z"}};

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

static int argmax(float *array, int size)
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

static void compress_sequence(int *sequence, int num_rows, std::vector<int> &compressed_sequence)
{
    compressed_sequence.push_back(sequence[0]);

    for (size_t i = 1; i < num_rows; i++)
    {
        if (sequence[i] != sequence[i - 1])
        {
            compressed_sequence.push_back(sequence[i]);
        }
    }
}

static void decode(int *token_ids, int num_rows, std::vector<std::string> &recognized_text)
{
    std::vector<int> compressed_token_ids;
    std::string token;
    compress_sequence(token_ids, num_rows, compressed_token_ids);
    for (int token_id : compressed_token_ids)
    {
        if (token_id <= 4)
        {
            if (token_id == 4)
            {
                recognized_text.push_back(" ");
            }
            continue;
        }

        token = tokenizer_dict[token_id].token;
        recognized_text.push_back(token);
    }
}

void audio_preprocess(audio_buffer_t *audio, std::vector<float> &audio_data)
{
    std::vector<float> ori_audio_data(audio->data, audio->data + audio->num_frames);
    pad_or_trim(ori_audio_data, audio_data, audio->num_frames, N_SAMPLES);
}

void post_process(float *output, std::vector<std::string> &recognized_text)
{
    int num_rows = OUTPUT_SIZE;
    int num_columns = VOCAB_NUM;

    int predicted_ids[num_rows];
    for (int i = 0; i < num_rows; i++)
    {
        int maxIndex = argmax(&output[i * num_columns], num_columns);
        predicted_ids[i] = maxIndex;
    }

    decode(predicted_ids, num_rows, recognized_text);
}