#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
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