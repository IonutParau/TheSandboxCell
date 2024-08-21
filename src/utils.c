#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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
