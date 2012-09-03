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
unsigned char *ClutBuf = NULL;

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
	printf("link () not implemented\n");
    return -1;
}

int ps2_close_game(void)
{

	browser_reset_path();

	video_packets_free();

	if(!is_loaded) {
		return 0;
	}

	FCEUI_CloseGame();

	is_loaded = 0;

	return 1;
}

int ps2_load_game(const char *path)
{
	settings_t settings;

	settings = settings_get();

	// customize for different modes
	video_packets_init();

	video_init_framebuffer(256,256);

	video_init_texbuffer(256,256,GS_PSM_8,GS_PSM_24);

	video_init_screen(0,0,256,240,0,settings.display.mode);
	video_init_draw_env(256,240);

	video_send_packet(256,256,(void*)XBuf,(void*)ClutBuf);
	video_draw_packet(256,256,GS_PSM_8,GS_PSM_24);
#ifdef SOUND_ON
	struct audsrv_fmt_t format;
	format.bits = 16;
	format.freq = 4800;
	format.channels = 1;
	audsrv_set_format(&format);
    audsrv_set_volume(100);
#endif
	if(!FCEUI_LoadGame(path))
	{
		ps2_close_game();
		return 0;
	}

	//ParseGIInput(GameInfo);
	//RefreshThrottleFPS();

	//if(!DriverInitialize(GameInfo)) {
	//	return(0);
	//}

	// set pal/ntsc
	//int id;
	//g_config->getOption("SDL.PAL", &id);

	//if(id)
		//FCEUI_SetVidSystem(1);
	//else
	FCEUI_SetVidSystem(NTSC_VID);

	FCEUI_SetGameGenie(0);
	FCEUI_DisableSpriteLimitation(1);
#ifdef SOUND_ON
	FCEUI_SetSoundVolume(1024);
#else
	FCEUI_SetSoundVolume(0);
#endif
	FCEUI_SetSoundQuality(0);
	FCEUI_SetLowPass(1);
	FCEUI_Sound(SAMPLERATE);
	//FCEUI_SetShowFPS(true);

	is_loaded = 1;

	//FCEUD_NetworkConnect();

	return 1;
}

void RenderFrame(const uint8 *frame)
{

	// Creates the dmachains, only needs to be done once
	// upload texture and clut
	video_send_texture();
	video_draw_texture();

    /* vsync and flip buffer */
    //video_sync_wait();
}
#ifdef SOUND_ON
short int soundbuf[2000];
void inline OutputSound(const int32 *tmpsnd, int32 ssize)
{
    //used as an example from the windows driver
    /*static int16 MBuffer[2 * 96000 / 50];  // * 2 for safety.
    int P;

    if(!bittage) {
        for(P=0;P<Count;P++)
            *(((uint8*)MBuffer)+P)=((int8)(Buffer[P]>>8))^128;
        RawWrite(MBuffer,Count);
    }
    else {
        for(P=0;P<Count;P++)
        MBuffer[P]=Buffer[P];
        //FCEU_printf("Pre: %d\n",RawCanWrite() / 2);
        RawWrite(MBuffer,Count * 2);
        //FCEU_printf("Post: %d\n",RawCanWrite() / 2);
     }*/

    int32 i = ssize;
    //s16 ssound[ssize]; //no need for an 2*ssized 8bit array with this
    //for (i=0;i<=ssize;i++) {
    while(i--) {
        //something[i]=((tmpsnd[i]>>8))^128; //for 8bit sound
        soundbuf[i]=tmpsnd[i];
    }
    audsrv_play_audio((const char *)soundbuf,ssize<<1);
}
#endif
void FCEUD_Update(const uint8 *XBuf, const int32 *tmpsnd, int32 ssize)
{
        RenderFrame(XBuf);
#ifdef SOUND_ON
        OutputSound(tmpsnd, ssize);
#endif
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

	ps2_close_game();
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

	// Move this to a driver initializing function
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

	// Change color format to BGR888
	// And fill extra entries for GS Clut Buffer
	// with copied versions of the palette
	// Maybe: Add 0x80 or lum value to final byte
#if 0
	int r,g,b;
    for(int i = 0; i< 64 ; i++ )
    {
        r =  ( NesPalette[ i ] & 0xff0000 )>>16;
        g =  ( NesPalette[ i ] & 0xff00 )>>8;
        b =  ( NesPalette[ i ] & 0xff )<<0;
        ClutBuf[ i ] = ((b<<16)|(g<<8)|(r<<0));
        ClutBuf[i+64] = ((b<<16)|(g<<8)|(r<<0));
        ClutBuf[i+128] = ((b<<16)|(g<<8)|(r<<0));
        ClutBuf[i+192] = ((b<<16)|(g<<8)|(r<<0));
    }
#endif

	// Initialize FCEUX
    ret = FCEUI_Initialize();

    if (ret != 1)
    {
        FCEUD_PrintError("FCEUI_Initialize returned error\n");
        return -1;
    }

	FCEUD_Message("Initialization complete!\n");

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

	}

    SleepThread();
	return 0;
}
