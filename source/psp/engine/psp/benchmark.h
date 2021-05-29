#ifdef __cplusplus
extern "C" {
#endif

#include <psptypes.h>
#include <time.h>
#include <psprtc.h>

extern u64 bm_first;
extern u64 bm_last;
extern u64 bm_time;

// https://stackoverflow.com/questions/12642830/can-i-define-a-function-inside-a-c-structure

void startBM();
void stopBM();
u64 getBM();
u64 getHighest();
u64 getAverage();

#ifdef __cplusplus
}
#endif