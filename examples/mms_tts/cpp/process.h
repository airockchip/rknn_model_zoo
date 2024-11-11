#ifndef _RKNN_MMS_TTS_DEMO_PROCESS_H_
#define _RKNN_MMS_TTS_DEMO_PROCESS_H_

#include "rknn_api.h"
#include "easy_timer.h"
#include "audio_utils.h"
#include <map>

#define VOCAB_NUM 38
#define SAMPLE_RATE 16000

#define MAX_LENGTH 200
#define PREDICTED_LENGTHS_MAX MAX_LENGTH * 2
#define PREDICTED_BATCH 256

#define INPUT_IDS_SIZE 1 * MAX_LENGTH
#define ATTENTION_MASK_SIZE 1 * MAX_LENGTH
#define LOG_DURATION_SIZE 1 * 1 * MAX_LENGTH
#define INPUT_PADDING_MASK_SIZE 1 * 1 * MAX_LENGTH
#define PRIOR_MEANS_SIZE 1 * MAX_LENGTH * 192
#define PRIOR_LOG_VARIANCES_SIZE 1 * MAX_LENGTH * 192
#define ATTN_SIZE 1 * 1 * PREDICTED_LENGTHS_MAX *MAX_LENGTH
#define OUTPUT_PADDING_MASK_SIZE 1 * 1 * PREDICTED_LENGTHS_MAX

void preprocess_input(const char *text, std::map<char, int> vocab, int vocab_size, int max_length, std::vector<int64_t> &input_id, std::vector<int64_t> &attention_mask);
void read_vocab(std::map<char, int> &vocab);
void middle_process(std::vector<float> log_duration, std::vector<float> input_padding_mask, std::vector<float> &attn, std::vector<float> &output_padding_mask, int &predicted_lengths_max_real);

#endif //_RKNN_MMS_TTS_DEMO_PROCESS_H_
