#include "ticking.h"
#include "../threads/tinycthread.h"
#include "subticks.h"
#include "grid.h"

volatile bool isGamePaused = true;
volatile bool isGameTicking = false;
volatile double tickTime = 0.0;
volatile double tickDelay = 0.15;
volatile bool multiTickPerFrame = true;
volatile size_t gameTPS = 0;
volatile bool isInitial = false;

static mtx_t renderingUselessMutex;
static cnd_t renderingTickUpdateSignal;

// Asynchronous updating
static int tsc_gridUpdateThread(void *_) {
    mtx_lock(&renderingUselessMutex);
    time_t last, now;
    size_t ticksInSecond = 0;
    time(&last);
    while(true) {
        if(!multiTickPerFrame) {
            cnd_wait(&renderingTickUpdateSignal, &renderingUselessMutex);
            mtx_unlock(&renderingUselessMutex);
        } else {
            if(tickTime < tickDelay || isGamePaused) {
                continue;
            }
        }
        // Nothing here is thread-safe except the waiting
        // The only thing keeping this from exploding is
        // high IQ code I wrote that I forgot to understand
        tickTime = 0.0;
        isGameTicking = true;
        if(isInitial) {
            isInitial = false;
            tsc_copyGrid(tsc_getGrid("initial"), currentGrid);
        }
        tsc_subtick_run();
        ticksInSecond++;
        time(&now);
        double delta = difftime(now, last);
        if(delta >= 1) {
            time(&last);
            gameTPS = ticksInSecond / delta;
            ticksInSecond = 0;
        }
        isGameTicking = false;
    }
}

void tsc_setupUpdateThread() {
    mtx_init(&renderingUselessMutex, mtx_plain);
    cnd_init(&renderingTickUpdateSignal);
    thrd_t updateThread;
    thrd_create(&updateThread, tsc_gridUpdateThread, NULL);
}

void tsc_signalUpdateShouldHappen() {
    cnd_signal(&renderingTickUpdateSignal);
}
