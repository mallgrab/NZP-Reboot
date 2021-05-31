#include "benchmark.h"

void StartBM(BM_Timer *t) {
    sceRtcGetCurrentTick(&t->bm_first);
}

void StopBM(BM_Timer *t) {
    sceRtcGetCurrentTick(&t->bm_last);
    t->bm_time = t->bm_last-t->bm_first;
    
    if (t->bm_highest < t->bm_time)
        t->bm_highest = t->bm_time;
}

u64 GetBM(BM_Timer *t) {
    return t->bm_time;
}

u64 GetHighest(BM_Timer *t) {
    return t->bm_highest;
}

u64 GetAverage(BM_Timer *t) {
    t->iter -= 1;
    t->bm_tmp += t->bm_time;

    if (t->iter < 0)
    {
        t->iter = 20;
        t->bm_average = t->bm_tmp / 20;
        t->bm_tmp = 0;
    }

    return t->bm_average;
}

BM_Timer LightPoint;
BM_Timer FindRadiusPF;
BM_Timer DrawAliasFrame;
BM_Timer InterpolateEntity;
BM_Timer RecursiveWorldNode;
BM_Timer RecursiveHullCheck;