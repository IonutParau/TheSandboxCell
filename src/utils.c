#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#ifdef linux
#include <dirent.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct tsc_intern_array_pool {
    char **str;
    size_t len;
} tsc_intern_array_pool;

typedef struct tsc_intern_hash_pool {
    tsc_intern_array_pool *arrays;
    size_t *hashes;
    size_t arrayc;
} tsc_intern_hash_pool;

tsc_intern_hash_pool *intern_pool = NULL;

static void tsc_intern_setup() {
    if(intern_pool != NULL) return;
    intern_pool = malloc(sizeof(tsc_intern_hash_pool));
    intern_pool->arrayc = 20;
    intern_pool->arrays = malloc(sizeof(tsc_intern_array_pool) * intern_pool->arrayc);
    intern_pool->hashes = malloc(sizeof(size_t) * intern_pool->arrayc);
    for(size_t i = 0; i < intern_pool->arrayc; i++) {
        intern_pool->hashes[i] = 0;
        tsc_intern_array_pool array;
        array.str = NULL;
        array.len = 0;
        intern_pool->arrays[i] = array;
    }
}

static const char *tsc_addToInternArray(tsc_intern_array_pool *pool, const char *str) {
    for(size_t i = 0; i < pool->len; i++) {
        if(tsc_streql(pool->str[i], str)) {
            return pool->str[i];
        }
    }
    char *s = tsc_strdup(str);
    size_t idx = pool->len++;
    pool->str = realloc(pool->str, sizeof(const char *) * pool->len);
    pool->str[idx] = s;
    return s;
}

// If we fuck up, we can measure how much we fucked up right here
// Higher is worse, as it measures the percentage of the discrepency.
// Ideally, it would be 0.
// Does not count unused arrays (aka length 0).
double tsc_strhashimbalance() {
    // TODO: implement the math. Now it just prints debug stuff
    for(size_t i = 0; i < intern_pool->arrayc; i++) {
        if(i == intern_pool->arrayc - 1) {
            printf("%lu\n", intern_pool->arrays[i].len);
        } else {
            printf("%lu ", intern_pool->arrays[i].len);
        }
    }
    return 0;
}

// New implementation not tested
// TODO: Stress test
const char *tsc_strintern(const char *str) {
    if(str == NULL) return NULL;
    tsc_intern_setup();
    size_t hash = tsc_strhash(str);
start:;
    size_t i = hash % intern_pool->arrayc;
    tsc_intern_array_pool *array = intern_pool->arrays + i;
    if(array->str == NULL) {
        intern_pool->hashes[i] = hash;
        return tsc_addToInternArray(array, str);
    }
    if(intern_pool->hashes[i] != hash) {
        // Collision!!!
        size_t arrayc = intern_pool->arrayc * 2;
        size_t *hashes = malloc(sizeof(size_t) * arrayc);
        tsc_intern_array_pool *arrays = malloc(sizeof(tsc_intern_array_pool) * arrayc);
        for(size_t i = 0; i < arrayc; i++) {
            hashes[i] = 0;
            arrays[i].str = NULL;
            arrays[i].len = 0;
        }
        for(size_t i = 0; i < intern_pool->arrayc; i++) {
            if(intern_pool->arrays[i].str == NULL) continue;
            size_t j = intern_pool->hashes[i] % arrayc;
            arrays[j] = intern_pool->arrays[i];
            hashes[j] = intern_pool->hashes[i];
        }
        free(intern_pool->hashes);
        free(intern_pool->arrays);
        intern_pool->arrayc = arrayc;
        intern_pool->arrays = arrays;
        intern_pool->hashes = hashes;
        goto start;
    }
    return tsc_addToInternArray(array, str);
}

int tsc_streql(const char *a, const char *b) {
    if(a == b) return true;
    return strcmp(a, b) == 0;
}

char *tsc_strdup(const char *str) {
    if(str == NULL) return NULL;
    char *buf = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(buf, str);
    return buf;
}

char *tsc_strcata(const char *a, const char *b) {
    char *buffer = malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
    strcpy(buffer, a);
    strcat(buffer, b);
    return buffer;
}

unsigned long tsc_strhash(const char *str) {
    // Jenkins one at a time hashing function as descripted in https://en.wikipedia.org/wiki/Jenkins_hash_function
    size_t i = 0;
    size_t hash = 0;
    while(str[i]) {
        hash += str[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 1;
    hash += hash << 15;
    return hash;
}

void tsc_memswap(void *a, void *b, size_t len) {
    uint8_t *aBytes = (uint8_t*)a;
    uint8_t *bBytes = (uint8_t*)b;
    for(size_t i = 0; i < len; i++) {
        uint8_t swp = aBytes[i];
        aBytes[i] = bBytes[i];
        bBytes[i] = swp;
    }
}

void tsc_pathfix(char *path) {
#ifdef _WIN32
    for(size_t i = 0; path[i] != '\0'; i++) {
        if(path[i] == '/') path[i] = '\\';
    }
#endif
    return;
}

const char *tsc_pathfixi(const char *path) {
    char *pathAlloc = tsc_strdup(path);
    tsc_pathfix(pathAlloc);
    const char *fixed = tsc_strintern(pathAlloc);
    free(pathAlloc);
    return fixed;
}

char tsc_pathsep() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

char *tsc_allocfile(const char *filepath, size_t *len) {
    FILE *file = fopen(filepath, "r");
    if(file == NULL) {
        if(len != NULL) *len = 0;
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if(len != NULL) *len = size;
    char *memory = malloc(sizeof(char) * (size + 1));
    fread(memory, sizeof(char), size, file);
    memory[size] = '\0';
    fclose(file);
    return memory;
}

void tsc_freefile(char *memory) {
    free(memory);
}

int tsc_hasfile(const char *path) {
    FILE *f = fopen(path, "r");
    if(f != NULL) fclose(f);
    return f != NULL;
}

const char *tsc_fextension(char *path) {
    for(size_t i = 0; path[i] != '\0'; i++) {
        if(path[i] == '.') {
            path[i] = '\0';
            return path + i + 1;
        }
    }
    return NULL;
}

char **tsc_dirfiles(const char *path, size_t *len) {
    char **files = malloc(sizeof(char *));
    files[0] = NULL;
    size_t i = 0;

#ifdef linux
    // Linux implementation
    DIR *dir = opendir(path);
    if(dir == NULL) {
        if(len != NULL) *len = 0;
        return files;
    }
    struct dirent *entry;

    while(true) {
        entry = readdir(dir);
        if(entry == NULL) break;
        // Dotfiles are hidden. This is because of . and ..
        if(*entry->d_name == '.') continue;
        files = realloc(files, sizeof(char *) * (i + 2));
        files[i] = tsc_strdup(entry->d_name);
        i++;
    }

    closedir(dir);
#endif
#ifdef _WIN32
    // Taken from https://learn.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory,
    // except we use FindFirstFileA because we love ASCII and UTF-16 is terrible.
    HANDLE hFind = INVALID_HANDLE_VALUE;
    static char pattern[256];
    snprintf(pattern, 256, "%s\\*", path);
    WIN32_FIND_DATAA ffd;
    hFind = FindFirstFileA(pattern, &ffd);
    if(hFind == INVALID_HANDLE_VALUE) {
        if(len != NULL) *len = 0;
        return files;
    }
    do {
        if(*ffd.cFileName == '.') continue;
        files = realloc(files, sizeof(char *) * (i + 2));
        files[i] = tsc_strdup(ffd.cFileName);
        i++;
    } while(FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind);
#endif
    if(len != NULL) *len = i;
    files[i] = NULL;
    return files;
}

void tsc_freedirfiles(char **dirfiles) {
    if(dirfiles == NULL) return;
    // null-terminated array
    for(size_t i = 0; dirfiles[i] != NULL; i++) {
        free(dirfiles[i]);
    }
    free(dirfiles);
}

void *tsc_malloc(size_t len) {
    return malloc(len);
}

void tsc_free(void *memory) {
    free(memory);
}

void *tsc_realloc(void *memory, size_t len) {
    return realloc(memory, len);
}

bool tsc_getBit(char *num, size_t bit) {
    size_t b = bit % 8;
    size_t i = bit / 8;
    size_t mask = 1 << b;

    return (num[i] & mask) != 0;
}

void tsc_setBit(char *num, size_t bit, bool value) {
    size_t b = bit % 8;
    size_t i = bit / 8;
    size_t mask = 1 << b;

    if(value) num[i] |= mask;
    else num[i] &= ~(char)value;
}

double tsc_mapNumber(double x, double min1, double max1, double min2, double max2) {
    double t = (x - min1) / (max1 - min1);
    return min2 + t * (max2 - min2);
}

bool tsc_isLittleEndian() {
    int x = 1;
    char *fuckTheCStandard = (void *)&x;
    return (*fuckTheCStandard) == 1;
}

char **tsc_alloclines(const char *text, size_t *len) {
    size_t lineCount = 0;

    for(size_t i = 0; text[i] != '\0'; i++) {
        if(text[i] == '\n') lineCount++;
    }

    if(len != NULL) *len = lineCount;

    char **lines = malloc(sizeof(char *) * (lineCount + 1));

    size_t j = 0;
    char *textcpy = tsc_strdup(text);
    char delim[2] = "\n";
    char *token = strtok(textcpy, delim);
    while(token != NULL) {
        lines[j] = tsc_strdup(token);
        j++;
        token = strtok(NULL, delim);
    }
    lines[j] = NULL;
    free(textcpy);

    return lines;
}

void tsc_freelines(char **lines) {
    size_t i = 0;
    while(lines[i] != NULL) free(lines[i++]);
    free(lines);
}

#ifdef TSC_POSIX

double tsc_clock() {
    struct timespec time;
    if(clock_gettime(CLOCK_MONOTONIC, &time) < 0) return 0; // oh no
    return time.tv_sec + ((double)time.tv_nsec) / 1e9;
}

#endif

#ifndef TSC_POSIX

// PESKY WINDOWS

int asprintf(char **s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    size_t len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *buffer = malloc(sizeof(char)*(len+1));
    if(buffer == NULL) {
        return -1;
    }
    buffer[len] = '\0';
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);

    *s = buffer;

    va_end(args);
    return len;
}

double tsc_clock() {
    LARGE_INTEGER frequency = {0};
    if(!QueryPerformanceFrequency(&frequency)) return 0;

    LARGE_INTEGER now = {0};
    if(!QueryPerformanceCounter(&now)) return 0;

    return (double)now.QuadPart / frequency.QuadPart;
}

#endif

tsc_arena_t tsc_tmp = {NULL};

static tsc_arena_chunk_t *tsc_aallocChunk(size_t len) {
    tsc_arena_chunk_t *chunk = malloc(sizeof(tsc_arena_chunk_t));
    if(chunk == NULL) return NULL;
    chunk->len = 0;
    chunk->capacity = len;
    chunk->buffer = malloc(len);
    if(chunk->buffer == NULL) {
        free(chunk);
        return NULL;
    }
    return chunk;
}

tsc_arena_t tsc_aempty() {
    return (tsc_arena_t) {NULL};
}

void *tsc_aallocAligned(tsc_arena_t *arena, size_t size, size_t align) {
    if(arena->chunk == NULL) {
        arena->chunk = tsc_aallocChunk(65536); // 64KiB by default cuz why not
    }
    tsc_arena_chunk_t *chunk = arena->chunk;

    while(chunk != NULL) {
        // align must be power of 2
        ptrdiff_t off = -(size_t)(chunk->buffer + chunk->len) & (align - 1);
        size_t idx = chunk->len + off;
        if(idx + size > chunk->capacity) {
            if(chunk->next == NULL) {
                // try to add another chunk.
                size_t needed = chunk->capacity;
                while(needed < size) {
                    needed *= 2;
                }
                tsc_arena_chunk_t *newChunk = tsc_aallocChunk(needed);
                chunk->next = newChunk;
                chunk = newChunk;
                continue;
            }
        }
        void *buffer = chunk->buffer + idx;
        chunk->len = idx + size;
        return buffer;
    }
    return NULL; // though should be unreachable
}

void *tsc_aalloc(tsc_arena_t *arena, size_t size) {
    // align of 2 pointers because... slices
    return tsc_aallocAligned(arena, size, sizeof(void *) * 2);
}

static const char *tsc_vasprintf(tsc_arena_t *arena, const char *fmt, va_list list1, va_list list2) {
    int len = vsnprintf(NULL, 0, fmt, list1);
    if(len < 0) return NULL; // format error
    char *buffer = tsc_aallocAligned(arena, len, sizeof(char));
    if(buffer == NULL) return NULL;
    vsprintf(buffer, fmt, list2);
    return buffer;
}

const char *tsc_asprintf(tsc_arena_t *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args2;
    va_start(args2, fmt);
    const char *s = tsc_vasprintf(arena, fmt, args, args2);
    va_end(args);
    va_end(args2);
    return s;
}

const char *tsc_tsprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args2;
    va_start(args2, fmt);
    const char *s = tsc_vasprintf(&tsc_tmp, fmt, args, args2);
    va_end(args);
    va_end(args2);
    return s;
}

void tsc_areset(tsc_arena_t *arena) {
    tsc_arena_chunk_t *chunk = arena->chunk;
    while(chunk != NULL) {
        chunk->len = 0;
        chunk = chunk->next;
    }
}

void tsc_aclear(tsc_arena_t *arena) {
    tsc_arena_chunk_t *current = arena->chunk;
    while(current != NULL) {
        tsc_arena_chunk_t *chunk = current;
        current = chunk->next;

        free(chunk->buffer);
        free(chunk);
    }
    arena->chunk = NULL;
}

size_t tsc_acount(tsc_arena_t *arena) {
    size_t s = 0;
    tsc_arena_chunk_t *chunk = arena->chunk;
    while(chunk != NULL) {
        s += chunk->capacity;
        chunk = chunk->next;
    }
    return s;
}

size_t tsc_aused(tsc_arena_t *arena) {
    size_t s = 0;
    tsc_arena_chunk_t *chunk = arena->chunk;
    while(chunk != NULL) {
        s += chunk->len;
        chunk = chunk->next;
    }
    return s;
}

void *tsc_tallocAligned(size_t size, size_t align) {
    return tsc_aallocAligned(&tsc_tmp, size, align);
}

void *tsc_talloc(size_t size) {
    return tsc_aalloc(&tsc_tmp, size);
}
