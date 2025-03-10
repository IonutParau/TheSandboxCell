#include <stddef.h>
#include <stdbool.h>
#include "grid.h"

extern volatile bool isGamePaused;
extern volatile bool isGameTicking;
extern volatile double tickTime;
extern volatile double tickDelay;
extern volatile size_t tickCount;
extern volatile bool multiTickPerFrame;
extern volatile bool onlyOneTick;
extern volatile size_t gameTPS;
extern volatile bool isInitial;
extern volatile bool storeExtraGraphicInfo;
extern char *volatile initialCode;

void tsc_setupUpdateThread();
void tsc_signalUpdateShouldHappen();
