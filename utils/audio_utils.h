#ifndef _RKNN_MODEL_ZOO_AUDIO_UTILS_H_
#define _RKNN_MODEL_ZOO_AUDIO_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float *data;
    int num_frames;
    int num_channels;
    int sample_rate;
} audio_buffer_t;

/**
 * @brief Read audio file
 * 
 * @param path [in] Audio path
 * @param audio [out] Read audio
 * @return int 0: success; -1: error
 */
int read_audio(const char* path, audio_buffer_t* audio);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _RKNN_MODEL_ZOO_AUDIO_UTILS_H_