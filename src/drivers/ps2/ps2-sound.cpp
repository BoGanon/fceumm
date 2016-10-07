#include <audsrv.h>

#include "driver.h"

short int soundbuf[2000];

void ps2_audio_init(void)
{
	struct audsrv_fmt_t format;

	format.bits = 16;
	format.freq = 4800;
	format.channels = 1;

	audsrv_set_format(&format);
    audsrv_set_volume(100);
}

void ps2_audio_deinit(void)
{
	audsrv_stop_audio();
}


void ps2_audio_play(const int16 *tmpsnd, int32 ssize)
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

   // int32 i = ssize;
    //s16 ssound[ssize]; //no need for an 2*ssized 8bit array with this
    //for (i=0;i<=ssize;i++) {
   // while(i--) {
        //something[i]=((tmpsnd[i]>>8))^128; //for 8bit sound
        //soundbuf[i]=tmpsnd[i];
   // }
    audsrv_wait_audio(ssize<<1);
    audsrv_play_audio((const char *)tmpsnd,ssize<<1);//soundbuf,ssize<<1);
}
