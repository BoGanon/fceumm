#include "driver.h"
#include <time.h>

void FCEUD_PrintError(const char *s)
{
    printf("Error: %s",s);
}

void FCEUD_Message(const char *s)
{
    printf("%s",s);
}

extern "C" uint64 FCEUD_GetTime(void)
{
    return clock();
}

extern "C" uint64 FCEUD_GetTimeFreq(void)
{
	return 576000;
}

