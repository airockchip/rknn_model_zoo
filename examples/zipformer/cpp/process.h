#ifndef _RKNN_ZIPFORMER_DEMO_PROCESS_H_
#define _RKNN_ZIPFORMER_DEMO_PROCESS_H_

// #define TIMING_DISABLED // if you don't need to print the time used, uncomment this line of code

#include "rknn_api.h"
#include "easy_timer.h"
#include "kaldi-native-fbank/csrc/online-feature.h"

#define VOCAB_NUM 6257
#define SAMPLE_RATE 16000
#define N_MELS 80
#define N_SEGMENT 103
#define ENCODER_OUTPUT_T 24
#define DECODER_DIM 512
#define ENCODER_INPUT_SIZE N_MELS *N_SEGMENT
#define ENCODER_OUTPUT_SIZE ENCODER_OUTPUT_T *DECODER_DIM
#define JOINER_OUTPUT_SIZE 6254
#define N_OFFSET 96
#define CONTEXT_SIZE 2

#define VOCAB_PATH "./model/vocab.txt"

typedef struct
{
    int index;
    char *token;
} VocabEntry;

int get_kbank_frames(knf::OnlineFbank *fbank, int frame_index, int segment, float *frames);
void convert_nchw_to_nhwc(float *src, float *dst, int N, int channels, int height, int width);
int argmax(float *array);
void replace_substr(std::string &str, const std::string &from, const std::string &to);
int read_vocab(const char *fileName, VocabEntry *vocab);

#endif //_RKNN_ZIPFORMER_DEMO_PROCESS_H_
