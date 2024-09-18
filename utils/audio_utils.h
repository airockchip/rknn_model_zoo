#ifndef _RKNN_MODEL_ZOO_AUDIO_UTILS_H_
#define _RKNN_MODEL_ZOO_AUDIO_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    float *data;
    int num_frames;
    int num_channels;
    int sample_rate;
} audio_buffer_t;

/**
 * @brief Reads an audio file into a buffer.
 *
 * @param path [in] Path to the audio file.
 * @param audio [out] Pointer to the audio buffer structure that will store the read data.
 * @return int 0 on success, -1 on error.
 */
int read_audio(const char *path, audio_buffer_t *audio);

/**
 * @brief Saves audio data to a WAV file.
 *
 * @param path [in] Path to the output WAV file.
 * @param data [in] Pointer to the audio data to be saved.
 * @param num_frames [in] Number of frames in the audio data.
 * @param sample_rate [in] Sampling rate of the audio data.
 * @param num_channels [in] Number of channels in the audio data.
 * @return int 0 on success, -1 on error.
 */
int save_audio(const char *path, float *data, int num_frames, int sample_rate, int num_channels);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _RKNN_MODEL_ZOO_AUDIO_UTILS_H_