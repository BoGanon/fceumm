#include <image.h>

#include "driver.h"

extern uint8* ClutBuf;

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

