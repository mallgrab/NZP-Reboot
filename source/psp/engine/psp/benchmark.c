#include "benchmark.h"

u64 bm_first;
u64 bm_last;
u64 bm_time;
u64 bm_highest;
u64 bm_average;
u64 bm_tmp;
int iter = 20;

void startBM() {
    sceRtcGetCurrentTick(&bm_first);
}

void stopBM() {
    sceRtcGetCurrentTick(&bm_last);
    bm_time = bm_last-bm_first;
    
    if (bm_highest < bm_time)
        bm_highest = bm_time;
}

u64 getBM() {
    return bm_time;
}

u64 getHighest() {
    return bm_highest;
}

u64 getAverage() {
    iter -= 1;
    bm_tmp += bm_time;

    if (iter < 0)
    {
        iter = 20;
        bm_average = bm_tmp / 20;
        bm_tmp = 0;
    }

    return bm_average;
}