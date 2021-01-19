/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include "ticks.h"

#if WINDOWS
#  include <windows.h>
/* force windows stamp function to run on core 1. From MSDN:
         On a multiprocessor computer, it should not matter which processor
         is called. However, you can get different results on different
         processors due to bugs in the basic input/output system (BIOS)
         or the hardware abstraction layer (HAL). To specify processor affinity
         for a thread, use the SetThreadAffinityMask function.
*/
static bool safeMode=false;
static LARGE_INTEGER gFrequency;
static bool gFreqSet=false;
#else
#include <time.h>
#endif



namespace SC3::Time {

// set safe mode (also init the freq)
void setSafeMode() {
#if WINDOWS
    safeMode=true;
    if(!gFreqSet)
    {
        QueryPerformanceFrequency(&gFrequency);
        gFreqSet = true;
    }
#endif
}

void getCurrentTimestamp(Timestamp *val) {
#if WINDOWS
    DWORD_PTR oldMask=0;
    LARGE_INTEGER ticks;

    if(!gFreqSet)
    {
        QueryPerformanceFrequency(&gFrequency);
        gFreqSet = true;
    }

    QueryPerformanceCounter(&ticks);
    if(safeMode) // clear affinity mask
        SetThreadAffinityMask(GetCurrentThread(), oldMask);

    *val=(int64_t)(((double)ticks.QuadPart / gFrequency.QuadPart) * 1000000);

#else
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    *val=(tv.tv_sec*1000000) + (tv.tv_nsec / 1000);
#endif
}

void getTimestampDiff(Timestamp *a, Timestamp *b, Timestamp *diff)
{
    *diff=*a-*b;
    if(*diff < 0)
        *diff *= -1;
}

} // namespace