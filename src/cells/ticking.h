#include <stddef.h>
#include <stdbool.h>

extern volatile bool isGamePaused;
extern volatile bool isGameTicking;
extern volatile double tickTime;
extern volatile double tickDelay;
extern volatile bool multiTickPerFrame;
extern volatile bool onlyOneTick;
extern volatile size_t gameTPS;
extern volatile bool isInitial;

void tsc_setupUpdateThread();
void tsc_signalUpdateShouldHappen();
