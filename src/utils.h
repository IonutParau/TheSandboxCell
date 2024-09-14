#ifndef TSC_UTILS
#define TSC_UTILS

#include <stddef.h>
#include <stdbool.h>

// Based off https://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
        #define TSC_WINDOWS
   #else
        #error "Windows 32-bit is not supported"
   #endif
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        #error "iPhone Emulators are not supported"
    #elif TARGET_OS_MACCATALYST
        // I guess?
        #define TSC_MACOS
    #elif TARGET_OS_IPHONE
        #error "iPhone are not supported"
    #elif TARGET_OS_MAC
        #define TSC_MACOS
    #else
        #error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    #error "Android is not supported"
#elif __linux__
    #define TSC_LINUX
#endif

#if __unix__ // all unices not caught above
    // Unix
    #define TSC_UNIX
    #define TSC_POSIX
#elif defined(_POSIX_VERSION)
    // POSIX
    #define TSC_POSIX
#endif

// x64 gives us 64 bit pointers, but in virtual memory, they become 48-bit pointers.
// 12 bits for a 4kb index within a page, 4 indexes 9 bits each which tell the CPU how to traverse the page table.
// The remaining 16 bits must be empty in case the 5th index is ever implemented, but it can be used to store extra info.
#if defined(__x86_64) || defined(_M_X64)
    #define TSC_PTR_HAS_SHORT 1
#else
    #define TSC_PTR_HAS_SHORT 0
#endif

const char *tsc_strintern(const char *str);
// hideapi
double tsc_strhashimbalance();
// hideapi
int tsc_streql(const char *a, const char *b);
char *tsc_strdup(const char *str);
char *tsc_strcata(const char *a, const char *b);
unsigned long tsc_strhash(const char *str);

void tsc_memswap(void *a, void *b, size_t len);

// Replaces the / with \ on Windows
void tsc_pathfix(char *path);
const char *tsc_pathfixi(const char *path);
char tsc_pathsep();

char *tsc_allocfile(const char *path, size_t *len);
void tsc_freefile(char *memory);
int tsc_hasfile(const char *path);
// You must free path. The extension comes out of that memory.
// It replaces the first . if any with a null terminator.
// NULL if there is no extension.
const char *tsc_fextension(char *path);

char **tsc_dirfiles(const char *path, size_t *len);
void tsc_freedirfiles(char **dirfiles);

void *tsc_malloc(size_t len);
void *tsc_realloc(void *buffer, size_t len);
void tsc_free(void *buffer);

bool tsc_getBit(char *num, size_t bit);
void tsc_setBit(char *num, size_t bit, bool value);

unsigned char tsc_getUnusedPointerByte(void *pointer);
void *tsc_getPointerWithoutByte(void *pointer);
void *tsc_setUnusedPointerByte(void *pointer, unsigned char byte);
// x64 only
unsigned short tsc_getUnusedPointerShort(void *pointer);
void *tsc_getPointerWithoutShort(void *pointer);
// x64 only
void *tsc_setUnusedPointerShort(void *pointer, unsigned short byte);

#endif
