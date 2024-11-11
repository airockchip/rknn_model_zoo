#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include "audio_utils.h"

int read_audio(const char *path, audio_buffer_t *audio)
{
    SNDFILE *infile;
    SF_INFO sfinfo = {0};

    infile = sf_open(path, SFM_READ, &sfinfo);
    if (!infile)
    {
        fprintf(stderr, "Error: failed to open file '%s': %s\n", path, sf_strerror(NULL));
        return -1;
    }

    audio->num_frames = sfinfo.frames;
    audio->num_channels = sfinfo.channels;
    audio->sample_rate = sfinfo.samplerate;
    audio->data = (float *)malloc(audio->num_frames * audio->num_channels * sizeof(float));
    if (!audio->data)
    {
        fprintf(stderr, "Error: failed to allocate memory.\n");
        sf_close(infile);
        return -1;
    }

    sf_count_t num_read_frames = sf_readf_float(infile, audio->data, audio->num_frames);
    if (num_read_frames != audio->num_frames)
    {
        fprintf(stderr, "Error: failed to read all frames. Expected %ld, got %ld.\n", (long)audio->num_frames, (long)num_read_frames);
        free(audio->data);
        sf_close(infile);
        return -1;
    }

    sf_close(infile);

    return 0;
}

int save_audio(const char *path, float *data, int num_frames, int sample_rate, int num_channels)
{
    SNDFILE *outfile;
    SF_INFO sfinfo = {0};

    sfinfo.frames = num_frames;
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = num_channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    outfile = sf_open(path, SFM_WRITE, &sfinfo);
    if (!outfile)
    {
        fprintf(stderr, "Error: failed to open file '%s' for writing: %s\n", path, sf_strerror(NULL));
        return -1;
    }

    sf_count_t num_written_frames = sf_writef_float(outfile, data, num_frames);
    if (num_written_frames != num_frames)
    {
        fprintf(stderr, "Error: failed to write all frames. Expected %ld, wrote %ld.\n", (long)num_frames, (long)num_written_frames);
        sf_close(outfile);
        return -1;
    }

    sf_close(outfile);

    return 0;
}

int resample_audio(audio_buffer_t *audio, int original_sample_rate, int desired_sample_rate)
{
    int original_length = audio->num_frames;
    int out_length = round(original_length * (double)desired_sample_rate / (double)original_sample_rate);
    printf("resample_audio: %d HZ -> %d HZ \n", original_sample_rate, desired_sample_rate);

    float *resampled_data = (float *)malloc(out_length * sizeof(float));
    if (!resampled_data)
    {
        return -1;
    }

    for (int i = 0; i < out_length; ++i)
    {
        double src_index = i * (double)original_sample_rate / (double)desired_sample_rate;
        int left_index = (int)floor(src_index);
        int right_index = (left_index + 1 < original_length) ? left_index + 1 : left_index;
        double fraction = src_index - left_index;
        resampled_data[i] = (1.0f - fraction) * audio->data[left_index] + fraction * audio->data[right_index];
    }

    audio->num_frames = out_length;
    free(audio->data);
    audio->data = resampled_data;

    return 0;
}

int convert_channels(audio_buffer_t *audio)
{

    int original_num_channels = audio->num_channels;
    printf("convert_channels: %d -> %d \n", original_num_channels, 1);

    float *converted_data = (float *)malloc(audio->num_frames * sizeof(float));
    if (!converted_data)
    {
        return -1;
    }

    for (int i = 0; i < audio->num_frames; ++i)
    {
        float left = audio->data[i * 2];
        float right = audio->data[i * 2 + 1];
        converted_data[i] = (left + right) / 2.0f;
    }

    audio->num_channels = 1;
    free(audio->data);
    audio->data = converted_data;

    return 0;
}
