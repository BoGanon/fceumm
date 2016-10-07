#include <kernel.h>

#include <string.h>
#include <image.h>

#include <gs_psm.h>

#include "interface/include/settings.h"
#include "interface/include/video.h"
#include "interface/include/browser.h"

#include "driver.h"
extern uint8* XBuf;
extern unsigned int *ClutBuf;
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

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{

	ClutBuf[(swizzle_8[index])] = (((u32)0x80<<24)|(b<<16)|(g<<8)|(r<<0));
}

void FCEUD_GetPalette(uint8 i,uint8 *r, uint8 *g, uint8 *b)
{
	*r = ClutBuf[swizzle_8[i]] & 0xff;
	*g = (ClutBuf[swizzle_8[i]] & 0x00ff00) >> 8;
	*b = (ClutBuf[swizzle_8[i]] & 0xff0000) >> 16;
}

void ps2_video_init(void)
{
	static int do_once = 0;
	settings_t settings;

	settings = settings_get();

#if 0
	// Change color format to BGR888
	// And fill extra entries for GS Clut Buffer
	// with copied versions of the palette
	// Maybe: Add 0x80 or lum value to final byte
	// Initialize to 0
	memset(ClutBuf,0,16*16*4);
	int r,g,b;
    for(int i = 0; i< 64 ; i++ )
    {
		r =  ( nes_palette[ i ] & 0xff0000 )>>16;
		g =  ( nes_palette[ i ] & 0xff00 )>>8;
		b =  ( nes_palette[ i ] & 0xff )<<0;
		ClutBuf[(swizzle_8[i])] = ((b<<16)|(g<<8)|(r<<0));
		ClutBuf[(swizzle_8[i])+64] = ((b<<16)|(g<<8)|(r<<0));
		ClutBuf[(swizzle_8[i])+128] = ((b<<16)|(g<<8)|(r<<0));
		ClutBuf[(swizzle_8[i])+192] = ((b<<16)|(g<<8)|(r<<0));
	}
#endif

	video_init_framebuffer(256,256);

	video_init_texbuffer(256,256,GS_PSM_8,GS_PSM_24);

	video_init_screen(0,0,256,240,0,settings.display.mode);

	video_init_draw_env(256,240);

	if (!do_once)
	{
		// I have no idea why but this seems to only work once
		// Seems to work for other emulators just fine
		// Maybe the GIF knows what's up
		video_send_packet(256,256,(void*)XBuf,(void*)ClutBuf);

	}
	video_draw_packet(256,240,GS_PSM_8,GS_PSM_24);

	do_once = 1;
}

void ps2_video_deinit(void)
{
	//video_packets_free();

}
