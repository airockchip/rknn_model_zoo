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

/**
 * @brief Resamples audio data to a desired sample rate.
 *
 * This function adjusts the sample rate of the provided audio data from 
 * the original sample rate to the desired sample rate. The audio data 
 * is assumed to be in a format compatible with the processing logic.
 *
 * @param audio [in/out] Pointer to the audio buffer structure containing 
 *                       the audio data to be resampled.
 * @param original_sample_rate [in] The original sample rate of the audio data.
 * @param desired_sample_rate [in] The target sample rate to resample the audio data to.
 * @return int 0 on success, -1 on error.
 */
int resample_audio(audio_buffer_t *audio, int original_sample_rate, int desired_sample_rate);

/**
 * @brief Converts audio data to a single channel (mono).
 *
 * This function takes two-channel audio data and converts it to single 
 * channel (mono) by averaging the channels or using another merging strategy.
 * The audio data will be modified in place.
 *
 * @param audio [in/out] Pointer to the audio buffer structure containing 
 *                       the audio data to be converted.
 * @return int 0 on success, -1 on error.
 */
int convert_channels(audio_buffer_t *audio);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _RKNN_MODEL_ZOO_AUDIO_UTILS_H_