#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include "audio_utils.h"

int read_audio(const char* path, audio_buffer_t* audio) {
    const char *filename = path;
    SNDFILE *infile;
    SF_INFO sfinfo = {0};
    
    infile = sf_open(filename, SFM_READ, &sfinfo);
    if (!infile) {
        fprintf(stderr, "Error: failed to open file '%s': %s\n", filename, sf_strerror(NULL));
        return -1;
    }

    audio->num_frames = sfinfo.frames;
    audio->num_channels = sfinfo.channels;
    audio->sample_rate = sfinfo.samplerate;
    audio->data = (float *)malloc(audio->num_frames * audio->num_channels * sizeof(float));
    if (!audio->data) {
        fprintf(stderr, "Error: failed to allocate memory.\n");
        sf_close(infile);
        return -1;
    }

    sf_count_t num_read_frames = sf_readf_float(infile, audio->data, audio->num_frames);
    if (num_read_frames != audio->num_frames) {
        fprintf(stderr, "Error: failed to read all frames. Expected %ld, got %ld.\n", (long)audio->num_frames, (long)num_read_frames);
        free(audio->data);
        sf_close(infile);
        return -1;
    }

    sf_close(infile);

    return 0;
}