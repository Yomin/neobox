/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "neobox_def.h"
#include "neobox_sound.h"

#define FORMAT_TAG_PCM 0x0001

struct wav_riff
{
    char id[4];     // "RIFF"
    uint32_t size;  // file length -8
    char type[4];   // "WAVE"
};

struct wav_fmt
{
    char id[4];             // "fmt "
    uint32_t size;          // size of following values
    uint16_t format_tag;    // 0x0001 canonical non compressed PCM
    uint16_t channels;
    uint32_t samples_per_sec;
    uint32_t avg_bytes_per_sec;
    uint16_t block_align;   // PCM: channels*((bits_per_sample+7)/8)
    uint16_t bits_per_sample;
};

struct wav_data
{
    char id[4];     // "data"
    uint32_t size;
};

struct thread_state
{
    pthread_t thread;
    FILE* audio_file;
    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t period_frames;
    struct wav_fmt audio_format;
    uint32_t audio_size;
    int play, loop, delay;
    long audio_pos;
};


extern struct neobox_global neobox;


int audio_read(void *dst, int size, FILE* file)
{
    int ret;
    
    if(!(ret = fread(dst, size, 1, file)))
    {
        if(ferror(file))
        {
            VERBOSE(fprintf(stderr, "[NEOBOX] Failed to read audio file\n"));
            return 1;
        }
        else
        {
            VERBOSE(printf("[NEOBOX] Audio file empty\n"));
            return 2;
        }
    }
    return 0;
}

void* neobox_sound_playback(void *vstate)
{
    struct thread_state *state = vstate;
    int ret;
    unsigned int data_size, size, buf_size, sample_size;
    char *buf, *sample;
    
    snd_pcm_uframes_t frames;
    
    sample_size = state->audio_format.channels*
        (state->audio_format.bits_per_sample/8);
    sample = malloc(sample_size);
    
    buf_size = state->period_frames*sample_size;
    buf = malloc(buf_size);
    
    VERBOSE(printf("[NEOBOX] playing sound\n"));
    
    if((ret = snd_pcm_prepare(state->pcm_handle)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to prepare audio device: %s\n", snd_strerror(ret)));
        goto exit;
    }
    
    while(state->play)
    {
        size = buf_size;
        data_size = state->audio_size;
        frames = state->period_frames;
        
        while(data_size && state->play)
        {
            if(data_size < size)
            {
                size = data_size;
                frames = data_size/(state->audio_format.channels*
                    (state->audio_format.bits_per_sample/8));
            }
            
            if(audio_read(buf, size, state->audio_file))
                goto exit;
            
            if((ret = snd_pcm_writei(state->pcm_handle, buf, frames)) == -EPIPE)
            {
                if((ret = snd_pcm_prepare(state->pcm_handle)) < 0)
                {
                    VERBOSE(fprintf(stderr, "[NEOBOX] Failed to prepare audio device: %s\n", snd_strerror(ret)));
                    goto exit;
                }
            }
            else if(ret < 0)
            {
                VERBOSE(fprintf(stderr, "[NEOBOX] Failed to write to audio device: %s\n", snd_strerror(ret)));
                goto exit;
            }
            
            data_size -= size;
        }
        
        if(!state->play || !state->loop)
            break;
        
        if(fseek(state->audio_file, state->audio_pos, SEEK_SET) == -1)
        {
            VERBOSE(perror("[NEOBOX] Failed to seek audio file\n"));
            goto exit;
        }
        
        memcpy(sample, &buf[size-sample_size], sample_size);
        
        for(data_size=0; data_size<buf_size; data_size += sample_size)
            memcpy(&buf[data_size], sample, sample_size);
        
        size = buf_size;
        frames = state->period_frames;
        data_size = state->delay*state->audio_format.samples_per_sec*
            state->audio_format.channels*(state->audio_format.bits_per_sample/8);
        
        while(data_size && state->play)
        {
            if(data_size < size)
            {
                size = data_size;
                frames = data_size/(state->audio_format.channels*
                    (state->audio_format.bits_per_sample/8));
            }
            
            if((ret = snd_pcm_writei(state->pcm_handle, buf, frames)) == -EPIPE)
            {
                if((ret = snd_pcm_prepare(state->pcm_handle)) < 0)
                {
                    VERBOSE(fprintf(stderr, "[NEOBOX] Failed to prepare audio device: %s\n", snd_strerror(ret)));
                    goto exit;
                }
            }
            else if(ret < 0)
            {
                VERBOSE(fprintf(stderr, "[NEOBOX] Failed to write to audio device: %s\n", snd_strerror(ret)));
                goto exit;
            }
            
            data_size -= size;
        }
    }
    
exit:
    if(!state->play)
        snd_pcm_drop(state->pcm_handle);
    else
        snd_pcm_drain(state->pcm_handle);
    snd_pcm_close(state->pcm_handle);
    free(buf);
    free(sample);
    free(state);
    
    pthread_exit(0);
    
    return 0;
}

void* neobox_sound_play(const char *file, int loop, int delay)
{
    pthread_attr_t attr;
    struct thread_state *state;
    
    int ret, format;
    
    struct wav_riff riff;
    struct wav_data data;
    
    snd_pcm_hw_params_t *params;
	
	state = malloc(sizeof(struct thread_state));
	state->loop = loop;
	state->delay = delay;
	state->play = 1;
	
    if(!(state->audio_file = fopen(file, "r")))
    {
        VERBOSE(perror("[NEOBOX] Failed to open audio file"));
        goto exit_sfree;
    }
    
    if(audio_read(&riff, sizeof(struct wav_riff), state->audio_file))
        goto exit_fclose;
    
    if(strncmp("RIFF", riff.id, 4) || strncmp("WAVE", riff.type, 4))
    {
        VERBOSE(printf("[NEOBOX] Unsupported format\n"));
        goto exit_fclose;
    }
    
    if((state->audio_pos = ftell(state->audio_file)) == -1)
    {
        VERBOSE(perror("[NEOBOX] Failed to get audio file position"));
        goto exit_fclose;
    }
    
    while(1)
    {
        switch(audio_read(&state->audio_format, sizeof(struct wav_data), state->audio_file))
        {
        case 1:
            goto exit_fclose;
        case 2:
            VERBOSE(printf("[NEOBOX] 'fmt' header not found\n"));
            goto exit_fclose;
        }
        
        if(!strncmp("fmt ", state->audio_format.id, 4))
        {
            if(audio_read((char*)&state->audio_format+sizeof(struct wav_data),
                sizeof(struct wav_fmt)-sizeof(struct wav_data), state->audio_file))
            {
                goto exit_fclose;
            }
            break;
        }
    }
    
    if(fseek(state->audio_file, state->audio_pos, SEEK_SET) == -1)
    {
        VERBOSE(perror("[NEOBOX] Failed to seek audio file\n"));
        goto exit_fclose;
    }
    
    while(1)
    {
        switch(audio_read(&data, sizeof(struct wav_data), state->audio_file))
        {
        case 1:
            goto exit_fclose;
        case 2:
            VERBOSE(printf("[NEOBOX] 'data' header not found\n"));
            goto exit_fclose;
        }
        
        if(!strncmp("data", data.id, 4))
            break;
    }
    
    state->audio_size = data.size;
    
    if((state->audio_pos = ftell(state->audio_file)) == -1)
    {
        VERBOSE(perror("[NEOBOX] Failed to get audio file position"));
        goto exit_fclose;
    }
    
    if(state->audio_format.format_tag != FORMAT_TAG_PCM)
    {
        VERBOSE(printf("[NEOBOX] Unsupported format tag\n"));
        goto exit_fclose;
    }
    
    switch(state->audio_format.bits_per_sample)
    {
    case 8:
        format = SND_PCM_FORMAT_U8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16_LE;
        break;
    default:
        VERBOSE(printf("[NEOBOX] Unsupported bits per sample\n"));
        goto exit_fclose;
    }
    
    if((ret = snd_pcm_open(&state->pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to open audio device: %s\n", snd_strerror(ret)));
        goto exit_fclose;
    }
    
    if((ret = snd_pcm_hw_params_malloc(&params)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to alloc hw_param: %s\n", snd_strerror(ret)));
        goto exit_pclose;
    }
    
    if((ret = snd_pcm_hw_params_any(state->pcm_handle, params)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to init hw_param: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params_set_access(state->pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set access: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params_set_format(state->pcm_handle, params, format)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set format: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params_set_channels(state->pcm_handle, params, state->audio_format.channels)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set channels: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params_set_rate_near(state->pcm_handle, params, &state->audio_format.samples_per_sec, 0)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set rate: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params(state->pcm_handle, params)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to set hw_params: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    if((ret = snd_pcm_hw_params_get_period_size(params, &state->period_frames, 0)) < 0)
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to get period size: %s\n", snd_strerror(ret)));
        goto exit_hfree;
    }
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
     
    if(pthread_create(&state->thread, &attr, neobox_sound_playback, state))
    {
        VERBOSE(fprintf(stderr, "[NEOBOX] Failed to create pthread\n"));
        goto exit_afree;
    }
    
    pthread_attr_destroy(&attr);
    snd_pcm_hw_params_free(params);
    snd_config_update_free_global();
    
    return state;
    
exit_afree:
    pthread_attr_destroy(&attr);
exit_hfree:
    snd_pcm_hw_params_free(params);
exit_pclose:
    snd_pcm_close(state->pcm_handle);
    snd_config_update_free_global();
exit_fclose:
    fclose(state->audio_file);
exit_sfree:
    free(state);
    
    return 0;
}

void neobox_sound_stop(void *thread)
{
    struct thread_state *state = thread;
    
    state->play = 0;
}
