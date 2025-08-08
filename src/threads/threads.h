#include "../utils.h"

#if defined(__STDC_NO_THREADS__) || defined(TSC_WINDOWS) // fuck you Windows
#include "tinycthread.h"
#else
#include <threads.h>
#endif
