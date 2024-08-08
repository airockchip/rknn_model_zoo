#ifndef _RKNN_YAMNET_DEMO_PROCESS_H_
#define _RKNN_YAMNET_DEMO_PROCESS_H_

#include "rknn_api.h"
#include "easy_timer.h"

#define LABEL_NUM 521
#define SAMPLE_RATE 16000
#define CHUNK_LENGTH 3
#define N_SAMPLES CHUNK_LENGTH *SAMPLE_RATE // AUDIO_LENGTH
#define N_ROWS 6                            // Modify this value according to output[2].shape[0]
#define LABEL_PATH "./model/yamnet_class_map.txt"

typedef struct
{
    int index;
    char *token;
} ResultEntry;

typedef struct
{
    int index;
    char *token;
} LabelEntry;

int audio_preprocess(audio_buffer_t *audio, float *audio_pad_or_trim);
int post_process(float *scores, LabelEntry *label, ResultEntry *result);
int read_label(LabelEntry *label);

#endif //_RKNN_YAMNET_DEMO_PROCESS_H_
