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

static int is_loaded = 0;

extern unsigned char *ClutBuf;
void ps2_video_init(void);
void ps2_video_render(const uint8*);
void ps2_video_deinit(void);
void ps2_audio_init(void);
void ps2_audio_play(const int32*, int32);

// AspiringSquire's Real Palette (C)
/*
u32 nes_palette[64] =
{
    0x6c6c6c, 0x00268e, 0x0000a8, 0x400090, 0x700070, 0x780040, 0x700000, 0x621600,
    0x442400, 0x343400, 0x005000, 0x004444, 0x004060, 0x000000, 0x101010, 0x101010,
    0xbababa, 0x205cdc, 0x3838ff, 0x8020f0, 0xc000c0, 0xd01474, 0xd02020, 0xac4014,
    0x7c5400, 0x586400, 0x008800, 0x007468, 0x00749c, 0x202020, 0x101010, 0x101010,
    0xffffff, 0x4ca0ff, 0x8888ff, 0xc06cff, 0xff50ff, 0xff64b8, 0xff7878, 0xff9638,
    0xdbab00, 0xa2ca20, 0x4adc4a, 0x2ccca4, 0x1cc2ea, 0x585858, 0x101010, 0x101010,
	0xffffff, 0xb0d4ff, 0xc4c4ff, 0xe8b8ff, 0xffb0ff, 0xffb8e8, 0xffc4c4, 0xffd4a8,
    0xffe890, 0xf0f4a4, 0xc0ffc0, 0xacf4f0, 0xa0e8ff, 0xc2c2c2, 0x202020, 0x101010
};


// swizzled
u32 nes_palette[64] =
{
    0x6c6c6c, 0x00268e, 0x0000a8, 0x400090, 0x700070, 0x780040, 0x700000, 0x621600,
    0xbababa, 0x205cdc, 0x3838ff, 0x8020f0, 0xc000c0, 0xd01474, 0xd02020, 0xac4014,
    0x442400, 0x343400, 0x005000, 0x004444, 0x004060, 0x000000, 0x101010, 0x101010,
    0x7c5400, 0x586400, 0x008800, 0x007468, 0x00749c, 0x202020, 0x101010, 0x101010,
    0xffffff, 0x4ca0ff, 0x8888ff, 0xc06cff, 0xff50ff, 0xff64b8, 0xff7878, 0xff9638,
    0xffffff, 0xb0d4ff, 0xc4c4ff, 0xe8b8ff, 0xffb0ff, 0xffb8e8, 0xffc4c4, 0xffd4a8,
    0xdbab00, 0xa2ca20, 0x4adc4a, 0x2ccca4, 0x1cc2ea, 0x585858, 0x101010, 0x101010,
    0xffe890, 0xf0f4a4, 0xc0ffc0, 0xacf4f0, 0xa0e8ff, 0xc2c2c2, 0x202020, 0x101010
};
*/

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
	ClutBuf = (uint8*)memalign(128,16*16*4);

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

	FCEUD_Message("Initialization complete!\n");

	return 0;

}

void ps2_close_game(void)
{

	browser_reset_path();
	ps2_video_deinit();

	if(is_loaded)
	{
		FCEUI_CloseGame();
	}

	is_loaded = 0;

}

int ps2_load_game(const char *path)
{

	ps2_video_init();

	if(!FCEUI_LoadGame(path))
	{
		ps2_close_game();
		return -1;
	}

	// set pal/ntsc
	FCEUI_SetVidSystem(NTSC_VID);
	FCEUI_SetGameGenie(0);
	FCEUI_DisableSpriteLimitation(1);
	FCEUI_SetSoundVolume(1024);
	FCEUI_SetSoundQuality(0);
	FCEUI_SetLowPass(1);
	FCEUI_Sound(SAMPLERATE);

	is_loaded = 1;

	//FCEUD_NetworkConnect();

	return 0;
}

void ps2_video_render(const uint8 *frame)
{

	// upload texture and clut
	video_send_texture();
	video_draw_texture();

    /* vsync and flip buffer */
    video_sync_wait();
}

void FCEUD_Update(const uint8 *XBuf, const int32 *tmpsnd, int32 ssize)
{

        ps2_video_render(XBuf);

        ps2_audio_play(tmpsnd, ssize);

}

void DoFun()
{
    uint8 *gfx;
    int32 *sound;
    int32 ssize;

    //while(!Get_NESInput() )
    while(1)
    {
        FCEUI_Emulate(&gfx, &sound, &ssize, 0);
        FCEUD_Update(gfx, sound, ssize);
    }

}


int main(int argc, char **argv)
{
    int ret;


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

	while(1)
	{
		interface_open();

		interface_run();

		interface_close();

		ps2_load_game(browser_get_path());

		if(is_loaded)
		{
			DoFun();
		}

		ps2_close_game();

	}

    SleepThread();
	return 0;
}
