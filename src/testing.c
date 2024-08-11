#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "testing.h"
#include "saving/test_saving.h"
#include <time.h>
#include "cells/grid.h"
#include "cells/subticks.h"
#include "saving/saving.h"
#include "threads/workers.h"
#include <raylib.h>
#include "api/api.h"

void tsc_test(const char *name) {
    printf("[ TESTING ] %s\n", name);
}

void tsc_vafail(const char *fmt, va_list args) {
    static char buffer[1024];
    vsnprintf(buffer, 1024, fmt, args);
    fprintf(stderr, "[ FAILED ] %s\n", buffer);
    //exit(1);
}

void __attribute__((format (printf, 1, 2))) tsc_fail(const char *fmt, ...) {
    static char buffer[1024];
    va_list args;
    va_start(args, fmt);
    tsc_vafail(fmt, args);
    va_end(args);
    //exit(1);
}

void __attribute__((format (printf, 2, 3))) tsc_assert(int check, const char *fmt, ...) {
    if(!check) {
        va_list args;
        va_start(args, fmt);
        tsc_vafail(fmt, args);
        va_end(args);
    }
}

int main(void) {
    srand(time(NULL));

    workers_setupBest();

    tsc_init_builtin_ids();
    
    tsc_subtick_addCore();
    tsc_saving_registerCore();
    tsc_loadDefaultCellBar();

    tsc_testSaving();
}
