#ifdef __cplusplus
extern "C" {
#endif

#include <psptypes.h>
#include <time.h>
#include <psprtc.h>

extern u64 bm_first;
extern u64 bm_last;
extern u64 bm_time;

typedef struct {
    u64 bm_first;
    u64 bm_last;
    u64 bm_time;
    u64 bm_highest;
    u64 bm_average;
    u64 bm_tmp;
    int iter;
} BM_Timer;

void StartBM(BM_Timer *t);
void StopBM(BM_Timer *t);
u64 GetBM(BM_Timer *t);
u64 GetHighest(BM_Timer *t);
u64 GetAverage(BM_Timer *t);

extern BM_Timer LightPoint;
extern BM_Timer FindRadiusPF;
extern BM_Timer DrawAliasFrame;
extern BM_Timer InterpolateEntity;
extern BM_Timer RecursiveWorldNode;
extern BM_Timer RecursiveHullCheck;

#ifdef __cplusplus
}
#endif