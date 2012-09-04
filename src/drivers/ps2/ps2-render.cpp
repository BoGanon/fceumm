#include <image.h>

#include <gs_psm.h>

#include "interface/include/settings.h"
#include "interface/include/video.h"
#include "interface/include/browser.h"

#include "driver.h"
#include "../../video.h"

unsigned char *ClutBuf = NULL;

#if 0
	// Change color format to BGR888
	// And fill extra entries for GS Clut Buffer
	// with copied versions of the palette
	// Maybe: Add 0x80 or lum value to final byte

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

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	ClutBuf[(swizzle_8[index]*4)+0] = r;
	ClutBuf[(swizzle_8[index]*4)+1] = g;
	ClutBuf[(swizzle_8[index]*4)+2] = b;
	ClutBuf[(swizzle_8[index]*4)+3] = 0x80;

}

void FCEUD_GetPalette(uint8 i,uint8 *r, uint8 *g, uint8 *b)
{
	*r = ClutBuf[(swizzle_8[i]*4)+0];
	*g = ClutBuf[(swizzle_8[i]*4)+1];
	*b = ClutBuf[(swizzle_8[i]*4)+2];
}

void ps2_video_init(void)
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

}

void ps2_video_deinit(void)
{

	video_packets_free();

}
