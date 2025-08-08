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

#if defined(__clang__) || defined(__GNUC__)
    #define TSC_LIKELY(x) __builtin_expect(!!(x), 1)
    #define TSC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define TSC_LIKELY(x) (x)
    #define TSC_UNLIKELY(x) (x)
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

bool tsc_isdir(const char *path);
char **tsc_dirfiles(const char *path, size_t *len);
void tsc_freedirfiles(char **dirfiles);
size_t tsc_countFilesRecursively(const char *path);

void *tsc_malloc(size_t len);
void *tsc_realloc(void *buffer, size_t len);
void tsc_free(void *buffer);

bool tsc_getBit(char *num, size_t bit);
void tsc_setBit(char *num, size_t bit, bool value);

double tsc_mapNumber(double x, double min1, double max1, double min2, double max2);

bool tsc_isLittleEndian();

char **tsc_alloclines(const char *text, size_t *len);
void tsc_freelines(char **lines);

// Returns in seconds
double tsc_clock();

#ifndef TSC_POSIX

int asprintf(char **s, const char *fmt, ...);

#endif

// hideapi

typedef struct tsc_arena_chunk_t {
    unsigned char *buffer;
    size_t len;
    size_t capacity;
    struct tsc_arena_chunk_t *next;
} tsc_arena_chunk_t;

// hideapi

typedef struct tsc_arena_t
// hideapi
{
    tsc_arena_chunk_t *chunk;
}
// hideapi
tsc_arena_t;

extern tsc_arena_t tsc_tmp;

tsc_arena_t tsc_aempty();
void *tsc_aallocAligned(tsc_arena_t *arena, size_t size, size_t align);
void *tsc_aalloc(tsc_arena_t *arena, size_t size);
const char *tsc_asprintf(tsc_arena_t *arena, const char *fmt, ...);
const char *tsc_tsprintf(const char *fmt, ...);
void tsc_areset(tsc_arena_t *arena);
void tsc_aclear(tsc_arena_t *arena);
size_t tsc_acount(tsc_arena_t *arena);
size_t tsc_aused(tsc_arena_t *arena);
void *tsc_tallocAligned(size_t size, size_t align);
void *tsc_talloc(size_t size);

double tsc_frand();

#endif
