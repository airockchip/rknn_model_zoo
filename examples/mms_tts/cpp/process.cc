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

#include "mms_tts.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <map>
#include <string.h>
#include <algorithm>
#include <numeric>

static int strlenarr(const char *arr)
{
    int count = 0;
    while (*arr++)
        count++;
    return count;
}

static void compute_output_padding_mask(std::vector<float> &output_padding_mask, int predicted_lengths_max_real, int predicted_lengths_max)
{
    std::transform(output_padding_mask.begin(), output_padding_mask.end(), output_padding_mask.begin(),
                   [predicted_lengths_max_real](int i)
                   {
                       return (float)(i < predicted_lengths_max_real);
                   });
}

static void compute_attn_mask(std::vector<float> &output_padding_mask, std::vector<float> &input_padding_mask,
                              std::vector<int> &attn_mask, int predicted_lengths_max, int input_padding_mask_size)
{
    std::transform(attn_mask.begin(), attn_mask.end(), attn_mask.begin(),
                   [&output_padding_mask, &input_padding_mask, predicted_lengths_max, input_padding_mask_size](int index)
                   {
                       int i = index / input_padding_mask_size;
                       int j = index % input_padding_mask_size;
                       return int(output_padding_mask[i] * input_padding_mask[j]);
                   });
}

static void compute_duration(const std::vector<float> &exp_log_duration, const std::vector<float> &input_padding_mask,
                             std::vector<float> &duration, float length_scale)
{
    std::transform(exp_log_duration.begin(), exp_log_duration.end(), input_padding_mask.begin(), duration.begin(),
                   [length_scale](float exp_log_val, float mask_val)
                   {
                       return ceil(exp_log_val * mask_val * length_scale);
                   });
}

static void compute_valid_indices(const std::vector<float> &cum_duration, std::vector<int> &valid_indices, int input_padding_mask_size, int predicted_lengths_max)
{
    std::vector<int> indices(valid_indices.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::for_each(indices.begin(), indices.end(),
                  [cum_duration, &valid_indices, predicted_lengths_max](int index)
                  {
                      int i = index / predicted_lengths_max;
                      int j = index % predicted_lengths_max;
                      valid_indices[index] = j < cum_duration[i] ? 1 : 0;
                  });
}

static std::vector<float> exp_vector(const std::vector<float> &vec)
{
    std::vector<float> result(vec.size());
    std::transform(vec.begin(), vec.end(), result.begin(), [](float v)
                   { return exp(v); });
    return result;
}

static std::vector<float> cumsum(const std::vector<float> &vec)
{
    std::vector<float> result(vec.size());
    std::partial_sum(vec.begin(), vec.end(), result.begin());
    return result;
}

static void transpose_mul(const std::vector<int> &input, int input_rows, int input_cols, std::vector<int> attn_mask, std::vector<float> &output)
{
    std::vector<int> indices(input.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::for_each(indices.begin(), indices.end(),
                  [&input, &attn_mask, &output, input_rows, input_cols](int index)
                  {
                      int i = index / input_cols;
                      int j = index % input_cols;
                      output[j * input_rows + i] = (float)(input[index] * attn_mask[j * input_rows + i]);
                  });
}

static void compute_pad_indices(const std::vector<int> &valid_indices, std::vector<int> &sliced_indices, int input_length, int output_length)
{
    int padded_length = input_length + 1;
    std::vector<int> padded_indices(padded_length * output_length, 0);

    std::copy(valid_indices.begin(), valid_indices.end(), padded_indices.begin() + output_length);

    std::copy(padded_indices.begin(), padded_indices.begin() + input_length * output_length, sliced_indices.begin());

    std::transform(valid_indices.begin(), valid_indices.end(), sliced_indices.begin(),
                   sliced_indices.begin(), std::minus<int>());
}

void read_vocab(std::map<char, int> &vocab)
{
    vocab = {
        {' ', 19}, {'\'', 1}, {'-', 14}, {'0', 23}, {'1', 15}, {'2', 28}, {'3', 11}, {'4', 27}, {'5', 35}, {'6', 36}, {'_', 30}, {'a', 26}, 
        {'b', 24}, {'c', 12}, {'d', 5}, {'e', 7}, {'f', 20}, {'g', 37}, {'h', 6}, {'i', 18}, {'j', 16}, {'k', 0}, {'l', 21}, {'m', 17}, {'n', 29}, 
        {'o', 22}, {'p', 13}, {'q', 34}, {'r', 25}, {'s', 8}, {'t', 33}, {'u', 4}, {'v', 32}, {'w', 9}, {'x', 31}, {'y', 3}, {'z', 2}, {u'\u2013', 10}};
}

void preprocess_input(const char *text, std::map<char, int> vocab, int vocab_size, int max_length, std::vector<int64_t> &input_ids,
                      std::vector<int64_t> &attention_mask)
{
    int text_len = strlenarr(text);
    int input_len = 0;

    for (int i = 0; i < text_len; i++)
    {
        char token = tolower(text[i]);
        int token_index = vocab[token];

        if (input_len < max_length - 2)
        {
            input_ids[input_len++] = 0;
            input_ids[input_len++] = token_index;
        }
        else
        {
            break;
        }
    }

    input_ids[input_len++] = 0;

    for (int i = 0; i < input_len; i++)
    {
        attention_mask[i] = 1;
    }
}

void middle_process(std::vector<float> log_duration, std::vector<float> input_padding_mask, std::vector<float> &attn,
                    std::vector<float> &output_padding_mask, int &predicted_lengths_max_real)
{

    float speaking_rate = 1.0f;
    float length_scale = 1.0f / speaking_rate;

    std::vector<float> duration(LOG_DURATION_SIZE);
    std::vector<float> exp_log_duration = exp_vector(log_duration);
    compute_duration(exp_log_duration, input_padding_mask, duration, length_scale);

    float predicted_length_sum = std::accumulate(duration.begin(), duration.end(), 0.0f);
    predicted_lengths_max_real = std::max(1.0f, predicted_length_sum);
    int predicted_lengths_max = PREDICTED_LENGTHS_MAX;
    compute_output_padding_mask(output_padding_mask, predicted_lengths_max_real, predicted_lengths_max);

    int input_padding_mask_size = MAX_LENGTH;
    std::vector<int> attn_mask(predicted_lengths_max * input_padding_mask_size);
    compute_attn_mask(output_padding_mask, input_padding_mask, attn_mask, predicted_lengths_max, input_padding_mask_size);

    std::vector<float> cum_duration = cumsum(duration);
    std::vector<int> valid_indices(input_padding_mask_size * predicted_lengths_max, 0);
    compute_valid_indices(cum_duration, valid_indices, input_padding_mask_size, predicted_lengths_max);

    std::vector<int> padded_indices(input_padding_mask_size * predicted_lengths_max, 0);
    compute_pad_indices(valid_indices, padded_indices, input_padding_mask_size, predicted_lengths_max);

    transpose_mul(padded_indices, input_padding_mask_size, predicted_lengths_max, attn_mask, attn);
}