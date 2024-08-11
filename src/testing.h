#ifndef TSC_TESTING_H
#define TSC_TESTING_H

#include <stdarg.h>

void tsc_test(const char *name);
void tsc_vafail(const char *fmt, va_list args); 
void __attribute__((format (printf, 1, 2))) tsc_fail(const char *fmt, ...);
void __attribute__((format (printf, 2, 3))) tsc_assert(int check, const char *fmt, ...);

#endif
