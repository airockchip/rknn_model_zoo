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

#include "whisper.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <fftw3.h>
#include <opencv2/opencv.hpp>

#define ENABLE_NEON 1

#if ENABLE_NEON
#include "arm_neon.h"
#endif

int read_mel_filters(const char *fileName, float *data, int max_lines)
{
    FILE *file;
    int line_count = 0;

    file = fopen(fileName, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    while (line_count < max_lines && fscanf(file, "%f", &data[line_count]) == 1)
    {
        line_count++;
    }

    fclose(file);

    return 0;
}

static void pad_x_mel(const std::vector<float> input, int rows_input, int cols_input, std::vector<float> &output, int cols_output)
{
    for (int i = 0; i < rows_input; ++i)
    {
        std::copy(input.begin() + i * cols_input, input.begin() + (i + 1) * cols_input, output.begin() + i * cols_output);
    }
}

static void hann_window(std::vector<float> &window, int length)
{
    for (int i = 0; i < length; i++)
    {
        window[i] = 0.5 * (1 - cos(2 * M_PI * i / (length - 1)));
    }
}

static void reflect_pad(const std::vector<float> &audio, std::vector<float> &padded_audio, int pad_width)
{

    std::copy(audio.begin(), audio.end(), padded_audio.begin() + pad_width);
    std::reverse_copy(audio.begin(), audio.begin() + pad_width, padded_audio.begin());
    std::reverse_copy(audio.end() - pad_width, audio.end(), padded_audio.end() - pad_width);
}

#if ENABLE_NEON
static void stfts_neon(const std::vector<float> &audio, int audio_length, int window_length, int hop_length, const std::vector<float> &window, fftwf_complex *stft_result, int num_frames)
{
    float *input = (float *)fftwf_malloc(sizeof(float) * window_length);
    fftwf_complex *output = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (window_length / 2 + 1));
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(window_length, input, output, FFTW_ESTIMATE);
    for (int i = 0; i < num_frames; i++)
    {
        int start = i * hop_length;
        int end = start + window_length;
        for (int j = 0; j < window_length - 3; j += 4)
        {
            if (start + j < audio_length)
            {
                float32x4_t in = vld1q_f32(audio.data() + start + j);
                float32x4_t win = vld1q_f32(window.data() + j);
                float32x4_t out = vmulq_f32(in, win);
                vst1q_f32(input + j, out);
            }
            else
            {
                vst1q_f32(input + j, vdupq_n_f32(0.0f));
            }
        }

        for (int j = window_length - window_length % 4; j < window_length; j++)
        {
            if (start + j < audio_length)
            {
                input[j] = audio[start + j] * window[j];
            }
            else
            {
                input[j] = 0.0f;
            }
        }

        fftwf_execute(plan);
        memcpy(stft_result + i * (window_length / 2 + 1), output, sizeof(fftwf_complex) * (window_length / 2 + 1));
    }

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);
}
#else
static void stfts(const std::vector<float> &audio, int audio_length, int window_length, int hop_length, const std::vector<float> &window, fftwf_complex *stft_result, int num_frames)
{
    float *input = (float *)fftwf_malloc(sizeof(float) * window_length);
    fftwf_complex *output = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (window_length / 2 + 1));
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(window_length, input, output, FFTW_ESTIMATE);

    for (int i = 0; i < num_frames; i++)
    {
        int start = i * hop_length;
        int end = start + window_length;

        for (int j = 0; j < window_length; j++)
        {
            if (start + j < audio_length)
            {
                input[j] = audio[start + j] * window[j];
            }
            else
            {
                input[j] = 0.0f;
            }
        }

        fftwf_execute(plan);
        memcpy(stft_result + i * (window_length / 2 + 1), output, sizeof(fftwf_complex) * (window_length / 2 + 1));
    }

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);
}
#endif

static float compute_magnitude(const fftwf_complex &value)
{
    return value[0] * value[0] + value[1] * value[1];
}

static void compute_magnitudes(fftwf_complex *stft_result, int num_mel_filters, int num_frames, std::vector<float> &magnitudes)
{
    int k = 0;
    for (int i = 0; i < num_mel_filters; i++)
    {
        for (int j = 0; j < num_frames - 1; j++)
        {
            magnitudes[k] = compute_magnitude(stft_result[i * num_frames + j]);
            k++;
        }
    }
}

static void clamp_and_log_max(std::vector<float> &mel_spec, int rows, int cols)
{
    float min_val = 1e-10;
    float scaling_factor = 1.0 / 4.0;
    float shift_value = 4.0;

    float max_val = mel_spec[0];
    for (int i = 0; i < rows * cols; ++i)
    {
        float value = mel_spec[i];
        value = (value < min_val) ? min_val : value;
        mel_spec[i] = log10f(value);

        if (mel_spec[i] > max_val)
            max_val = mel_spec[i];
    }

    float threshold = max_val - 8.0;
    for (int i = 0; i < rows * cols; ++i)
    {
        mel_spec[i] = (std::max(mel_spec[i], threshold) + shift_value) * scaling_factor;
    }
}

void transpose(fftwf_complex *input, int input_rows, int input_cols, fftwf_complex *output)
{
    for (int i = 0; i < input_rows; ++i)
    {
        for (int j = 0; j < input_cols; ++j)
        {
            int input_index = i * input_cols + j;
            int output_index = j * input_rows + i;

            output[output_index][0] = input[input_index][0];
            output[output_index][1] = input[input_index][1];
        }
    }
}

#if ENABLE_NEON
void matmul_by_neon(float *A, float *B, std::vector<float> &C, int ROWS_A, int COLS_A, int COLS_B)
{
    int k_start = COLS_A, k_end = 0;
    for (auto i = 0; i < ROWS_A; i++)
    {
        for (auto k = 0; k < COLS_A; k++)
        {
            if (A[i * COLS_A + k] != 0.0f)
            {
                k_start = k;
                break;
            }
        }
        for (auto k = COLS_A - 1; k > 0; k--)
        {
            if (A[i * COLS_A + k] != 0.0f)
            {
                k_end = k;
                break;
            }
        }

        int k_diff = k_end - k_start + 1;
        if (k_diff % 4)
        {
            k_diff = 4 - (k_diff % 4);
            if (k_start > k_diff)
                k_start -= k_diff;
            else if (k_end < (COLS_A - k_diff))
                k_end += k_diff;
        }
        k_diff = (k_end - k_start) % 4;
        for (auto j = 0; j < COLS_B; j++)
        {
            float32x4_t v_tot = vdupq_n_f32(0.0f);
            for (auto k = k_start; k <= k_end - 3; k += 4)
            {
                float32x4_t vq_mat1 = vld1q_f32(&A[i * COLS_A + k]);
                float32x4_t vq_mat2 = vld1q_f32(&B[k * COLS_B + j]);
                v_tot = vmlaq_f32(v_tot, vq_mat1, vq_mat2);
            }

            float32x2_t tmp = vadd_f32(vget_high_f32(v_tot), vget_low_f32(v_tot));
            C[i * COLS_B + j] = vget_lane_f32(tmp, 0) + vget_lane_f32(tmp, 1);
        }
    }
}
#else
void matmul_by_opencv(float *A, float *B, std::vector<float> &C, int ROWS_A, int COLS_A, int COLS_B)
{
    cv::Mat mat_A(ROWS_A, COLS_A, CV_32F, A);
    cv::Mat mat_B(COLS_A, COLS_B, CV_32F, B);
    cv::Mat mat_C(ROWS_A, COLS_B, CV_32F);
    cv::gemm(mat_A, mat_B, 1.0, cv::Mat(), 0.0, mat_C);
    memcpy(C.data(), mat_C.data, ROWS_A * COLS_B * sizeof(float));
}
#endif

static void log_mel_spectrogram(float *audio_data, int audio_length, int cur_num_frames_of_stfts, float *filters, std::vector<float> &mel_spec)
{
    std::vector<float> window(N_FFT);
    hann_window(window, N_FFT);

    std::vector<float> audio(audio_data, audio_data + audio_length);
    int padded_size = audio_length + N_FFT;
    std::vector<float> padded_audio(padded_size);
    reflect_pad(audio, padded_audio, N_FFT / 2);

    fftwf_complex *stfts_result = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * MELS_FILTERS_SIZE * cur_num_frames_of_stfts);
#if ENABLE_NEON
    stfts_neon(padded_audio, audio_length + N_FFT, N_FFT, HOP_LENGTH, window, stfts_result, cur_num_frames_of_stfts);
#else
    stfts(padded_audio, audio_length + N_FFT, N_FFT, HOP_LENGTH, window, stfts_result, cur_num_frames_of_stfts);
#endif

    fftwf_complex *stfts_result_t = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * MELS_FILTERS_SIZE * cur_num_frames_of_stfts);
    transpose(stfts_result, cur_num_frames_of_stfts, MELS_FILTERS_SIZE, stfts_result_t);

    std::vector<float> magnitudes(MELS_FILTERS_SIZE * (cur_num_frames_of_stfts - 1));
    compute_magnitudes(stfts_result_t, MELS_FILTERS_SIZE, cur_num_frames_of_stfts, magnitudes);

    int ROWS_A = N_MELS;
    int COLS_A = MELS_FILTERS_SIZE;
    int COLS_B = cur_num_frames_of_stfts - 1;
#if ENABLE_NEON
    matmul_by_neon(filters, magnitudes.data(), mel_spec, ROWS_A, COLS_A, COLS_B);
#else
    matmul_by_opencv(filters, magnitudes.data(), mel_spec, ROWS_A, COLS_A, COLS_B);
#endif

    clamp_and_log_max(mel_spec, ROWS_A, COLS_B);

    fftwf_free(stfts_result);
    fftwf_free(stfts_result_t);
}

void audio_preprocess(audio_buffer_t *audio, float *mel_filters, std::vector<float> &x_mel)
{
    int ret;
    int audio_length = audio->num_frames;
    std::vector<float> ori_audio_data(audio->data, audio->data + audio_length);

    if (audio_length >= MAX_AUDIO_LENGTH)
    {
        std::vector<float> trim_audio_data(MAX_AUDIO_LENGTH);
        std::copy(ori_audio_data.begin(), ori_audio_data.begin() + MAX_AUDIO_LENGTH, trim_audio_data.begin());
        int cur_num_frames_of_stfts = MAX_AUDIO_LENGTH / HOP_LENGTH + 1;
        log_mel_spectrogram(trim_audio_data.data(), MAX_AUDIO_LENGTH, cur_num_frames_of_stfts, mel_filters, x_mel);
    }
    else
    {
        int cur_num_frames_of_stfts = audio_length / HOP_LENGTH + 1;
        int x_mel_rows = N_MELS;
        int x_mel_cols = cur_num_frames_of_stfts - 1;
        int x_mel_cols_pad = MAX_AUDIO_LENGTH / HOP_LENGTH;
        std::vector<float> cur_x_mel(x_mel_rows * x_mel_cols, 0.0f);
        log_mel_spectrogram(ori_audio_data.data(), audio_length, cur_num_frames_of_stfts, mel_filters, cur_x_mel);
        pad_x_mel(cur_x_mel, x_mel_rows, x_mel_cols, x_mel, x_mel_cols_pad);
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
        vocab[count].token = strdup(strchr(line, ' ') + 1); // Get token after the first space
        vocab[count].index = atoi(line);                    // Get index before the first space
        count++;
    }

    fclose(fp);

    return 0;
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

int argmax(float *array)
{
    int start_index = (MAX_TOKENS - 1) * 1 * VOCAB_NUM;
    int max_index = 0;
    float max_value = array[start_index];
    for (int i = start_index + 1; i < start_index + VOCAB_NUM; i++)
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

static int32_t get_char_index(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A';
    }
    else if (c >= 'a' && c <= 'z')
    {
        return c - 'a' + ('Z' - 'A') + 1;
    }
    else if (c >= '0' && c <= '9')
    {
        return c - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    }
    else if (c == '+')
    {
        return 62;
    }
    else if (c == '/')
    {
        return 63;
    }

    std::cerr << "Unknown character " << static_cast<int>(c) << ", " << c << std::endl;
    exit(-1);
}

std::string base64_decode(const std::string &encoded_string)
{
    // see
    // https://github.com/ReneNyffenegger/cpp-base64/blob/master/base64.cpp#L243
    // https://github.com/k2-fsa/sherpa-onnx/blob/master/sherpa-onnx/csrc/base64-decode.cc
    if (encoded_string.empty())
    {
        std::cerr << "Empty string!" << std::endl;
        exit(-1);
    }

    int32_t output_length = static_cast<int32_t>(encoded_string.size()) / 4 * 3;

    std::string decoded_string;
    decoded_string.reserve(output_length);

    int32_t index = 0;
    while (index < static_cast<int32_t>(encoded_string.size()))
    {
        if (encoded_string[index] == '=')
        {
            return " ";
        }

        int32_t first_byte = (get_char_index(encoded_string[index]) << 2) + ((get_char_index(encoded_string[index + 1]) & 0x30) >> 4);
        decoded_string.push_back(static_cast<char>(first_byte));

        if (index + 2 < static_cast<int32_t>(encoded_string.size()) && encoded_string[index + 2] != '=')
        {
            int32_t second_byte = ((get_char_index(encoded_string[index + 1]) & 0x0f) << 4) + ((get_char_index(encoded_string[index + 2]) & 0x3c) >> 2);
            decoded_string.push_back(static_cast<char>(second_byte));

            if (index + 3 < static_cast<int32_t>(encoded_string.size()) && encoded_string[index + 3] != '=')
            {
                int32_t third_byte = ((get_char_index(encoded_string[index + 2]) & 0x03) << 6) + get_char_index(encoded_string[index + 3]);
                decoded_string.push_back(static_cast<char>(third_byte));
            }
        }
        index += 4;
    }

    return decoded_string;
}