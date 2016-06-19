/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           Flemming Frandsen <dren.dk@gmail.com>                               #
#                             Add VU meter OSD                                  #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

/*******************************************************************************#
#                                                                               #
#  Audio library                                                                #
#                                                                               #
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "../config.h"
#include "gviewaudio.h"
#include "gview.h"
#include "audio_portaudio.h"
#if HAS_PULSEAUDIO
  #include "audio_pulseaudio.h"
#endif

/*audio device data mutex*/
static __MUTEX_TYPE mutex = __STATIC_MUTEX_INIT;
#define __PMUTEX &mutex

#define AUDBUFF_NUM     80    /*number of audio buffers*/
#define AUDBUFF_FRAMES  1152  /*number of audio frames per buffer*/
static audio_buff_t *audio_buffers = NULL; /*pointer to buffers list*/
static int buffer_read_index = 0; /*current read index of buffer list*/
static int buffer_write_index = 0;/*current write index of buffer list*/

int verbosity = 0;
static int audio_api = AUDIO_PORTAUDIO;

/*
 * set verbosity
 * args:
 *   value - verbosity value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_set_verbosity(int value)
{
	verbosity = value;
}

/*
 * Lock the mutex
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void audio_lock_mutex()
{
	__LOCK_MUTEX( __PMUTEX );
}

/*
 * Unlock the mutex
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void audio_unlock_mutex()
{
	__UNLOCK_MUTEX( __PMUTEX );
}

/*
 * free audio buffers
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
static int audio_free_buffers()
{
	buffer_read_index = 0;
	buffer_write_index = 0;
	
	/*return if no buffers set*/
	if(!audio_buffers)
	{
		if(verbosity > 0)
			fprintf(stderr,"AUDIO: can't free audio buffers (audio_free_buffers): audio_buffers is null\n");
		return 0;
	}
		
	int i = 0;

	for(i = 0; i < AUDBUFF_NUM; ++i)
	{
		free(audio_buffers[i].data);
	}

	free(audio_buffers);
	audio_buffers = NULL;
}

/*
 * alloc a single audio buffer
 * args:
 *    audio_ctx - pointer to audio context data
 *
 * asserts:
 *    none
 *
 * returns: pointer to newly allocate audio buffer or NULL on error
 *   must be freed with audio_delete_buffer
 * data is allocated for float(32 bit) samples but can also store
 * int16 (16 bit) samples
 */
audio_buff_t *audio_get_buffer(audio_context_t *audio_ctx)
{
	if(audio_ctx->capture_buff_size <= 0)
	{
		fprintf(stderr, "AUDIO: (get_buffer) invalid capture_buff_size(%i)\n",
			audio_ctx->capture_buff_size);
		return NULL;
	}

	audio_buff_t *audio_buff = calloc(1, sizeof(audio_buff_t));
	if(audio_buff == NULL)
	{
		fprintf(stderr,"AUDIO: FATAL memory allocation failure (audio_get_buffer): %s\n", strerror(errno));
		exit(-1);
	}
	audio_buff->data = calloc(audio_ctx->capture_buff_size, sizeof(sample_t));
	if(audio_buff->data == NULL)
	{
		fprintf(stderr,"AUDIO: FATAL memory allocation failure (audio_get_buffer): %s\n", strerror(errno));
		exit(-1);
	}

	return audio_buff;
}

/*
 * deletes a single audio buffer
 * args:
 *    audio_buff - pointer to audio_buff_t data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_delete_buffer(audio_buff_t *audio_buff)
{
	if(!audio_buff)
		return;

	if(audio_buff->data)
		free(audio_buff->data);

	free(audio_buff);
}

/*
 * alloc audio buffers
 * args:
 *    audio_ctx - pointer to audio context data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
static int audio_init_buffers(audio_context_t *audio_ctx)
{
	if(!audio_ctx)
		return -1;

	/*don't allocate if no audio*/
	if(audio_api == AUDIO_NONE)
	{
		audio_buffers = NULL;
		return 0;
	}
	
	int i = 0;

	/*set the buffers size*/
	if(!audio_ctx->capture_buff_size)
		audio_ctx->capture_buff_size = audio_ctx->channels * AUDBUFF_FRAMES;

	if(audio_ctx->capture_buff)
		free(audio_ctx->capture_buff);

	audio_ctx->capture_buff = calloc(
		audio_ctx->capture_buff_size, sizeof(sample_t));
	if(audio_ctx->capture_buff == NULL)
	{
		fprintf(stderr,"AUDIO: FATAL memory allocation failure (audio_init_buffers): %s\n", strerror(errno));
		exit(-1);
	}
	
	/*free audio_buffers (if any)*/
	audio_free_buffers;

	audio_buffers = calloc(AUDBUFF_NUM, sizeof(audio_buff_t));
	if(audio_buffers == NULL)
	{
		fprintf(stderr,"AUDIO: FATAL memory allocation failure (audio_init_buffers): %s\n", strerror(errno));
		exit(-1);
	}

	for(i = 0; i < AUDBUFF_NUM; ++i)
	{
		audio_buffers[i].data = calloc(
			audio_ctx->capture_buff_size, sizeof(sample_t));
		if(audio_buffers[i].data == NULL)
		{
			fprintf(stderr,"AUDIO: FATAL memory allocation failure (audio_init_buffers): %s\n", strerror(errno));
			exit(-1);
		}
		audio_buffers[i].flag = AUDIO_BUFF_FREE;
	}

	return 0;
}

/*
 * fill a audio buffer data and move write index to next one
 * args:
 *   audio_ctx - pointer to audio context data
 *   ts - timestamp for end of data
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: none
 */
void audio_fill_buffer(audio_context_t *audio_ctx, int64_t ts)
{
	/*assertions*/
	assert(audio_ctx != NULL);

	if(verbosity > 3)
		printf("AUDIO: filling buffer ts:%" PRId64 "\n", ts);
	/*in nanosec*/
	uint64_t frame_length = NSEC_PER_SEC / audio_ctx->samprate;
	uint64_t buffer_length = frame_length * (audio_ctx->capture_buff_size / audio_ctx->channels);

	audio_ctx->current_ts += buffer_length; /*buffer end time*/

	audio_ctx->ts_drift = audio_ctx->current_ts - ts;

	/*get the current write indexed buffer flag*/
	audio_lock_mutex();
	int flag = audio_buffers[buffer_write_index].flag;
	audio_unlock_mutex();

	if(flag == AUDIO_BUFF_USED)
	{
		fprintf(stderr, "AUDIO: write buffer(%i) is still in use - dropping data\n", buffer_write_index);
		return;
	}

	/*write max_frames and fill a buffer*/
	memcpy(audio_buffers[buffer_write_index].data,
		audio_ctx->capture_buff,
		audio_ctx->capture_buff_size * sizeof(sample_t));
	/*buffer begin time*/
	audio_buffers[buffer_write_index].timestamp = audio_ctx->current_ts - buffer_length;

	audio_buffers[buffer_write_index].level_meter[0] = audio_ctx->capture_buff_level[0];
	audio_buffers[buffer_write_index].level_meter[1] = audio_ctx->capture_buff_level[1];

	audio_lock_mutex();
	audio_buffers[buffer_write_index].flag = AUDIO_BUFF_USED;
	NEXT_IND(buffer_write_index, AUDBUFF_NUM);
	audio_unlock_mutex();

}

/* saturate float samples to int16 limits*/
static int16_t clip_int16 (float in)
{
	//int16_t out = (int16_t) (in < -32768) ? -32768 : (in > 32767) ? 32767 : in;

	long lout =  lroundf(in);
	int16_t out = (lout < INT16_MIN) ? INT16_MIN : (lout > INT16_MAX) ? INT16_MAX: (int16_t) lout;
	return (out);
}

/*
 * get the next used buffer from the ring buffer
 * args:
 *   audio_ctx - pointer to audio context
 *   buff - pointer to an allocated audio buffer
 *   type - type of data (SAMPLE_TYPE_[INT16|FLOAT])
 *   mask - audio fx mask
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int audio_get_next_buffer(audio_context_t *audio_ctx, audio_buff_t *buff, int type, uint32_t mask)
{
	audio_lock_mutex();
	int flag = audio_buffers[buffer_read_index].flag;
	audio_unlock_mutex();

	if(flag == AUDIO_BUFF_FREE)
		return 1; /*all done*/

	/*aplly fx*/
	audio_fx_apply(audio_ctx, (sample_t *) audio_buffers[buffer_read_index].data, mask);

	/*copy data into requested format type*/
	int i = 0;
	switch(type)
	{
		case GV_SAMPLE_TYPE_FLOAT:
		{
			sample_t *my_data = (sample_t *) buff->data;
			memcpy( my_data, audio_buffers[buffer_read_index].data,
				audio_ctx->capture_buff_size * sizeof(sample_t));
			break;
		}
		case GV_SAMPLE_TYPE_INT16:
		{
			int16_t *my_data = (int16_t *) buff->data;
			sample_t *buff_p = (sample_t *) audio_buffers[buffer_read_index].data;
			for(i = 0; i < audio_ctx->capture_buff_size; ++i)
			{
				my_data[i] = clip_int16( (buff_p[i]) * INT16_MAX);
			}
			break;
		}
		case GV_SAMPLE_TYPE_FLOATP:
		{
			int j=0;

			float *my_data[audio_ctx->channels];
			sample_t *buff_p = (sample_t *) audio_buffers[buffer_read_index].data;

			for(j = 0; j < audio_ctx->channels; ++j)
				my_data[j] = (float *) (((float *) buff->data) +
					(j * audio_ctx->capture_buff_size/audio_ctx->channels));

			for(i = 0; i < audio_ctx->capture_buff_size/audio_ctx->channels; ++i)
				for(j = 0; j < audio_ctx->channels; ++j)
				{
					my_data[j][i] = *buff_p++;
				}
			break;
		}
		case GV_SAMPLE_TYPE_INT16P:
		{
			int j=0;

			int16_t *my_data[audio_ctx->channels];
			sample_t *buff_p = (sample_t *) audio_buffers[buffer_read_index].data;

			for(j = 0; j < audio_ctx->channels; ++j)
				my_data[j] = (int16_t *) (((int16_t *) buff->data) +
					(j * audio_ctx->capture_buff_size/audio_ctx->channels));

			for(i = 0; i < audio_ctx->capture_buff_size/audio_ctx->channels; ++i)
				for(j = 0; j < audio_ctx->channels; ++j)
				{
					my_data[j][i] = clip_int16((*buff_p++) * INT16_MAX);
				}
			break;
		}
	}

	buff->timestamp = audio_buffers[buffer_read_index].timestamp;

	buff->level_meter[0] = audio_buffers[buffer_read_index].level_meter[0];
	buff->level_meter[1] = audio_buffers[buffer_read_index].level_meter[1];

	audio_lock_mutex();
	audio_buffers[buffer_read_index].flag = AUDIO_BUFF_FREE;
	NEXT_IND(buffer_read_index, AUDBUFF_NUM);
	audio_unlock_mutex();

	return 0;
}

/*
 * audio initialization
 * args:
 *   api - audio API to use
 *           (AUDIO_NONE, AUDIO_PORTAUDIO, AUDIO_PULSE, ...)
 *   device - api device index to use (-1 - use api default)
 *
 * asserts:
 *   none
 *
 * returns: pointer to audio context (NULL if AUDIO_NONE)
 */
audio_context_t *audio_init(int api, int device)
{

	audio_context_t *audio_ctx = NULL;

	audio_api = api;

	switch(audio_api)
	{
		case AUDIO_NONE:
			break;

#if HAS_PULSEAUDIO
		case AUDIO_PULSE:
			audio_ctx = audio_init_pulseaudio();
			break;
#endif
		case AUDIO_PORTAUDIO:
		default:
			audio_ctx = audio_init_portaudio();
			break;
	}

	if(!audio_ctx)
		audio_api = AUDIO_NONE;
	
	/*set default api device*/
	audio_set_device(audio_ctx, device);
		
	return audio_ctx;
}

/*
 * set audio device
 * args:
 *   audio_ctx - pointer to audio context data
 *   index - device index (from device list) to set
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: none
 */
void audio_set_device(audio_context_t *audio_ctx, int index)
{
	switch(audio_api)
	{
		case AUDIO_NONE:
			break;

#if HAS_PULSEAUDIO
		case AUDIO_PULSE:
			audio_set_pulseaudio_device(audio_ctx, index);
			break;
#endif
		case AUDIO_PORTAUDIO:
		default:
			audio_set_portaudio_device(audio_ctx, index);
			break;
	}
}

/*
 * start audio stream capture
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: error code
 */
int audio_start(audio_context_t *audio_ctx)
{
	if(verbosity > 1)
		printf("AUDIO: starting audio capture\n");
	/*assertions*/
	assert(audio_ctx != NULL);

	/*alloc the ring buffer*/
	audio_init_buffers(audio_ctx);
	
	/*reset timestamp values*/
	audio_ctx->current_ts = 0;
	audio_ctx->last_ts = 0;
	audio_ctx->snd_begintime = 0;
	audio_ctx->ts_drift = 0;  

	int err = 0;

	switch(audio_api)
	{
		case AUDIO_NONE:
			break;

#if HAS_PULSEAUDIO
		case AUDIO_PULSE:
			err = audio_start_pulseaudio(audio_ctx);
			break;
#endif
		case AUDIO_PORTAUDIO:
		default:
			err = audio_start_portaudio(audio_ctx);
			break;
	}

	return err;
}

/*
 * stop audio stream capture
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: error code
 */
int audio_stop(audio_context_t *audio_ctx)
{
	int err =0;

	switch(audio_api)
	{
		case AUDIO_NONE:
			break;

#if HAS_PULSEAUDIO
		case AUDIO_PULSE:
			err = audio_stop_pulseaudio(audio_ctx);
			break;
#endif
		case AUDIO_PORTAUDIO:
		default:
			err = audio_stop_portaudio(audio_ctx);
			break;
	}

	/*free the ring buffer (if any)*/
	audio_free_buffers();
		
	return err;
}

/*
 * close and clean audio context
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void audio_close(audio_context_t *audio_ctx)
{
	audio_fx_close();

	switch(audio_api)
	{
		case AUDIO_NONE:
			break;

#if HAS_PULSEAUDIO
		case AUDIO_PULSE:
			audio_close_pulseaudio(audio_ctx);
			break;
#endif
		case AUDIO_PORTAUDIO:
		default:
			audio_close_portaudio(audio_ctx);
			break;
	}

	if(audio_buffers != NULL)
		audio_free_buffers();
}
