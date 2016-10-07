#include <stdio.h>
#include <iostream>

#include <audsrv.h>
#include <gs_psm.h>

#include <image.h>

#include "interface/include/browser.h"
#include "interface/include/interface.h"
#include "interface/include/init.h"
#include "interface/include/video.h"
#include "interface/include/settings.h"

// FCEU Headers
#include "../../driver.h"
#include "../../fceu.h"
#include "../../video.h"

// Misc. macros
#define NTSC_VID 0
#define PAL_VID  1

#ifdef SOUND_ON
#define SAMPLERATE 48000
#else
#define SAMPLERATE 0
#endif

#define PAL_RATE 51.0f
#define NTSC_RATE 61.0f
#define HSYNC_RATE 625
#define HSYNCS 10
extern void *_gp;

static int is_loaded = 0;

unsigned int *ClutBuf;

static u8 sound_thread_stack[0x40000] __attribute__ ((aligned(16)));
static int sound_thread_id = -1;

static u8 dispatcher_thread_stack[0x2000] __attribute__ ((aligned(16)));
static int dispatcher_thread_id = -1;

static int sound_sema = -1;
#define CLOCKS_PER_MS 576.0f

static float frametime = 0.0f;
static int samples_mixed = 0;
void ps2_set_frametime()
{
        if (FCEUI_GetCurrentVidSystem(NULL, NULL) == PAL_VID)
        {
                frametime = (1000.0f/PAL_RATE) * CLOCKS_PER_MS;
                samples_mixed = (int)(SAMPLERATE / PAL_RATE);
        }
        else
        {
                frametime = (1000.0f/NTSC_RATE) * CLOCKS_PER_MS;
                samples_mixed = (int)(SAMPLERATE / NTSC_RATE);
        }
}

static inline int __udelay(unsigned int usecs)
{

	register unsigned int loops_total = 0;
	register unsigned int loops_end   = usecs * 148;

	if (usecs > loops_end)
	{

		return -1;

	}

	asm volatile (".set noreorder\n\t"
				  "0:\n\t"
				  "beq %0,%2,0f\n\t"
				  "addiu %0,1\n\t"
				  "bne %0,%2,0b\n\t"
				  "addiu %0,1\n\t"
				  "0:\n\t"
				  ".set reorder\n\t"
				  :"=r" (loops_total)
				  :"0" (loops_total), "r" (loops_end));

	return 0;

}
static int frameskip = 0;
void ps2_sync_speed()
{

	static unsigned long last_ms = 0;
	unsigned long now_ms;
	unsigned long diff_ms;

	now_ms = clock();

	if (!last_ms)
	{
		last_ms = now_ms;
		return;
	}

	diff_ms = now_ms - last_ms;

	// Took too long to render
	if (diff_ms > frametime)
	{
		frameskip++;
	}
	else
	{
		frameskip = 0;
		while((now_ms - last_ms) < frametime)
		{
			__udelay(50);
			now_ms = clock();
		}
	}

	last_ms = now_ms;
}

inline int sound_mutex_lock(void)
{
	return(WaitSema(sound_sema));
}
inline int sound_mutex_unlock(void)
{
	return(SignalSema(sound_sema));
}

inline int sound_mutex_trylock(void)
{
	ee_sema_t sema;

	ReferSemaStatus(sound_sema,&sema);

	if (sema.count)
	{
		return 0;
	}

	return -1;
}


#if 0
#ifdef USE_THREADS
void S9xInitTimer()
{

	ee_sema_t sema;
	ee_thread_t thread;
	audsrv_set_volume(100);

	if (Settings.ThreadSound)
	{
		SetAlarm( 310, alarmfunction, NULL);
		ChangeThreadPriority(GetThreadId(), 29);

		sema.init_count = 1;
		sema.max_count = 1;
		sema.option = 0;
		sound_sema = CreateSema(&sema);
//		switch_sema = CreateSema(&sema);

//		main_thread_id = GetThreadId();

		thread.func = (void*)dispatcher;
		thread.stack = (void*)dispatcher_thread_stack;
		thread.stack_size = 0x2000;
		thread.gp_reg = &_gp;
		thread.initial_priority = 0;
		dispatcher_thread_id = CreateThread(&thread);
		StartThread (dispatcher_thread_id, NULL);

		thread.func = (void*)S9xProcessSound;
		thread.stack = (void*)sound_thread_stack;
		thread.stack_size = 0x40000;
		thread.gp_reg = &_gp;
		thread.initial_priority = 30;
		sound_thread_id = CreateThread(&thread);
		StartThread(sound_thread_id, NULL);
	}

}

void S9xDeinitTimer()
{
		if (Settings.ThreadSound)
	{
		TerminateThread(sound_thread_id);
		DeleteThread(sound_thread_id);
		DeleteSema(sound_sema);
//		DeleteSema(switch_sema);
	}

	sound_sema = -1;
	sound_thread_id = -1;

}
#endif
static volatile bool block_signal = false;
static volatile bool pending_signal = false;

static volatile bool block_generate_sound = false;
void ps2_generate_sound()
{

#ifdef OPTI
	int bytes_so_far = (samples_mixed_so_far << 1);
#endif

	if (bytes_so_far >= buffer_size)
	{
		return;
	}

#ifdef USE_THREADS
	if (ThreadSound)
	{
		if (block_generate_sound || sound_mutex_trylock())
		{
			return;
		}
	}
#endif
	block_signal = true;

	int byte_offset;
	int byte_count;

#ifndef OPTI
		if (so.stereo)
#endif
		{
			sample_count <<= 1;
		}
		byte_offset = bytes_so_far + so.play_position;

		do
		{
			int sc = sample_count;
			byte_count = sample_count;
#ifndef OPTI
			if (so.sixteen_bit)
#endif
				byte_count <<= 1;

			if ((byte_offset & SOUND_BUFFER_SIZE_MASK) + byte_count > SOUND_BUFFER_SIZE)
			{
				sc = SOUND_BUFFER_SIZE - (byte_offset & SOUND_BUFFER_SIZE_MASK);
				byte_count = sc;
#ifndef OPTI
				if (so.sixteen_bit)
#endif
				{
					sc >>= 1;
				}
			}
			if (bytes_so_far + byte_count > so.buffer_size)
			{
				byte_count = so.buffer_size - bytes_so_far;
				if (byte_count == 0)
				{
					break;
				}
				sc = byte_count;
#ifndef OPTI
				if (so.sixteen_bit)
#endif
				{
					sc >>= 1;
				}
			}
			S9xMixSamplesO (Buf, sc, byte_offset & SOUND_BUFFER_SIZE_MASK);
			so.samples_mixed_so_far += sc;
			sample_count -= sc;
#ifdef OPTI
			bytes_so_far = (so.samples_mixed_so_far << 1);
#else
			bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
	                       so.samples_mixed_so_far;
#endif
			byte_offset += byte_count;
		}
		while (sample_count > 0);
	}

	block_signal = FALSE;

#ifdef USE_THREADS
	if (Settings.ThreadSound)
	{
		sound_mutex_unlock();
	}
	else
#endif
		// This never evaluates TRUE
		if (pending_signal)
		{
			S9xProcessSound (NULL);
			pending_signal = FALSE;
		}
}
void ps2_process_sound(void*)
{
#ifdef USE_THREADS
	do
	{
#endif
#ifdef USE_THREADS
		//SleepThread();
		//WaitSema(switch_sema);
#endif
		int sample_count = so.buffer_size;
		int byte_offset;

#ifndef OPTI
		if (so.sixteen_bit)
#endif
		{
			sample_count >>= 1;
		}

#ifdef USE_THREADS
		sound_mutex_lock();
#else
		block_generate_sound = true;
#endif

		/* If we need more audio samples */
		if (samples_mixed_so_far < sample_count)
		{
#ifdef OPTI
			byte_offset = so.play_position + (so.samples_mixed_so_far << 1);
#else
				byte_offset = so.play_position + (so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
	                          so.samples_mixed_so_far);
#endif
#ifdef _EE
			S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far,
							byte_offset & SOUND_BUFFER_SIZE_MASK);
#else
			if (Settings.SoundSync == 2)
			{
				memset (Buf + (byte_offset & SOUND_BUFFER_SIZE_MASK), 0, sample_count - so.samples_mixed_so_far);
			}
			else
			{
				S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far, byte_offset & SOUND_BUFFER_SIZE_MASK);
			}
#endif
			so.samples_mixed_so_far = 0;
		}
		else
		{
			so.samples_mixed_so_far -= sample_count;
		}

#ifdef OPTI
		unsigned bytes_to_write = so.buffer_size;
#else
		unsigned bytes_to_write = sample_count;
		if (so.sixteen_bit)
		{
			bytes_to_write <<= 1;
		}
#endif

		byte_offset = so.play_position;
		so.play_position = (so.play_position + so.buffer_size) & SOUND_BUFFER_SIZE_MASK;

#ifdef USE_THREADS
		sound_mutex_unlock();
#endif
		block_generate_sound = false;

		/* Feed the samples to the soundcard until nothing is left */
		for(;;)
		{

			int I = bytes_to_write;

			if (byte_offset + I > SOUND_BUFFER_SIZE)
			{
				I = SOUND_BUFFER_SIZE - byte_offset;
			}

			if(I == 0)
			{
				break;
			}

			audsrv_wait_audio(I);
			audsrv_play_audio((char*)Buf+byte_offset,I);

			if (I > 0)
			{
				bytes_to_write -= I;
				byte_offset += I;
				byte_offset &= SOUND_BUFFER_SIZE_MASK; /* wrap */
			}
		}
		/* All data sent. */
#ifdef USE_THREADS
		//ReleaseWaitThread(main_thread_id);
		//PollSema(switch_sema);
		//WakeupThread(main_thread_id);
	} while (Settings.ThreadSound);
#endif

	return (NULL);
}

}
#endif /* if 0 */
void dispatcher(void* arg)
{
	while(1)
	{
		SleepThread();
	}
}

void alarmfunction(s32 id, u16 time, void *arg)
{
	iWakeupThread(dispatcher_thread_id);
	iRotateThreadReadyQueue(30);
	iSetAlarm(HSYNCS,alarmfunction,NULL);
}

void ps2_video_init(void);
void ps2_video_render(void*);
void ps2_video_deinit(void);
void ps2_audio_init(void);
void ps2_audio_deinit(void);
void ps2_audio_play(const int16*, int32);
void ps2_input_open_pads(void);
void ps2_input_close_pads(void);
void ps2_input_nes_init(void);
int ps2_get_nes_input(void);

// Prototype for link in unistd.h
extern "C" int link(const char *oldpath, const char *newpath)
{
	printf("link() not implemented\n");
    return -1;
}

int ps2_fceu_init(void)
{
	int ret;

	// Allocate 8 bpp screen buffer
	XBuf = (uint8*)memalign(128,1*256*256);

	if (!XBuf)
	{
		FCEUD_PrintError("XBuf allocation failed\n");
		return -1;
	}

	// Allocate 256 32 bpp color palette
	ClutBuf = (u32*)memalign(128,16*16*4);

	if (!ClutBuf)
	{
		FCEUD_PrintError("ClutBuf allocation failed\n");
		return -1;
	}

	// Initialize to 0
	memset(ClutBuf,0,16*16*4);

	// Initialize FCEUX
    ret = FCEUI_Initialize();

    if (ret != 1)
    {
        FCEUD_PrintError("FCEUI_Initialize returned error\n");
        return -1;
    }

	// set pal/ntsc
	FCEUI_SetVidSystem(NTSC_VID);
	FCEUI_SetGameGenie(1);
	FCEUI_DisableSpriteLimitation(1);
	FCEUI_SetSoundVolume(1024);
	FCEUI_SetSoundQuality(0);
	FCEUI_SetLowPass(1);
	FCEUI_Sound(SAMPLERATE);
	FCEUI_SetBaseDirectory("mass:/FCEUMM");
	FCEUD_Message("Initialization complete!\n");

	return 0;

}

void ps2_close_game(void)
{

	browser_reset_path();

	ps2_video_deinit();
	ps2_audio_deinit();
	ps2_input_close_pads();

	if(is_loaded)
	{
		FCEUI_CloseGame();
	}

	is_loaded = 0;

}

int ps2_load_game(const char *path)
{

	ps2_video_init();

	ps2_input_open_pads();

	if(!FCEUI_LoadGame(path))
	{
		ps2_close_game();
		return -1;
	}

	is_loaded = 1;
	FCEUI_SelectState(1);
	ps2_set_frametime();
	//FCEUD_NetworkConnect();

	return 0;
}

void ps2_video_render(void*)
{
	//while(1)
	{
		//sound_mutex_lock();
		// upload texture and clut
		video_send_texture();
		video_draw_texture();
		//sound_mutex_unlock();

		/* vsync and flip buffer */
		video_sync_wait();
	}
		//SleepThread();
}

void ps2_init_threads()
{

	ee_sema_t sema;
	ee_thread_t thread;
	audsrv_set_volume(100);

	SetAlarm( HSYNCS, alarmfunction, NULL);
	ChangeThreadPriority(GetThreadId(), 29);

	sema.init_count = 1;
	sema.max_count = 1;
	sema.option = 0;
	sound_sema = CreateSema(&sema);
//		switch_sema = CreateSema(&sema);

//		main_thread_id = GetThreadId();

	thread.func = (void*)dispatcher;
	thread.stack = (void*)dispatcher_thread_stack;
	thread.stack_size = 0x2000;
	thread.gp_reg = &_gp;
	thread.initial_priority = 0;
	dispatcher_thread_id = CreateThread(&thread);
	StartThread (dispatcher_thread_id, NULL);

	thread.func = (void*)ps2_video_render;
	thread.stack = (void*)sound_thread_stack;
	thread.stack_size = 0x40000;
	thread.gp_reg = &_gp;
	thread.initial_priority = 30;
	sound_thread_id = CreateThread(&thread);
	StartThread(sound_thread_id, NULL);

}

void ps2_deinit_threads()
{
	TerminateThread(sound_thread_id);
	DeleteThread(sound_thread_id);
	DeleteSema(sound_sema);
	//DeleteSema(switch_sema);

	sound_sema = -1;
	sound_thread_id = -1;

}

void FCEUD_Update(const uint8 *gfx, const int16 *tmpsnd, int32 ssize)
{

        ps2_video_render(NULL);

        ps2_audio_play(tmpsnd, ssize);

}
static int skip = 0;
static int skipc = 0;

void DoFun()
{
    uint8 *gfx;
    int16 *sound;
    int32 ssize;

	ChangeThreadPriority(GetThreadId(),30);

    while(!ps2_get_nes_input())
    {
#if 0
    	if(frameskip > 0)
		{
			skip = 1;

			if(skipc >= frameskip)
			{
				skipc = 0;
				skip = 0;
			}
			else
			{
				skipc++;
			}
		}
#endif

		//if(!sound_mutex_trylock())
		{
			FCEUI_Emulate(&gfx, &sound, &ssize, skip);
			FCEUD_Update(gfx, sound, ssize);
			ps2_sync_speed();
		}
    }

}

int main(int argc, char **argv)
{
    int ret;
	settings_t settings;

	settings = settings_get();

	// Stops hardlocking when cin/cerr/cout etc. are used
	std::ios::sync_with_stdio(false);

	// Parses args for bootpath
	parse_args(argc,argv);

	init("fceux.cfg");
	init_sound_modules(NULL);

	ret = ps2_fceu_init();
	ps2_audio_init();

	if(ret < 0)
	{
		printf("Initialization failed\n");
	}

	video_packets_init();

	while(1)
	{
		interface_open();

		interface_run();

		interface_close();

		ps2_load_game(browser_get_path());
		//ps2_init_threads();
		if(is_loaded)
		{
			ps2_input_nes_init();
			DoFun();
		}
		//ps2_deinit_threads();
		ps2_close_game();
	}

    SleepThread();
	return 0;
}
