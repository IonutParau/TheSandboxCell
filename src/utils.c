#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

tsc_intern_array_pool *intern_pool = NULL;

static void tsc_intern_setup() {
    if(intern_pool != NULL) return;
    intern_pool = malloc(sizeof(tsc_intern_array_pool));
    intern_pool->len = 0;
    intern_pool->str = NULL;
}

const char *tsc_strintern(const char *str) {
    if(str == NULL) return NULL;
    tsc_intern_setup();
    for(size_t i = 0; i < intern_pool->len; i++) {
        if(tsc_streql(intern_pool->str[i], str)) {
            return intern_pool->str[i];
        }
    }
    char *entry = tsc_strdup(str);
    size_t idx = intern_pool->len++;
    intern_pool->str = realloc(intern_pool->str, sizeof(char *) * intern_pool->len);
    intern_pool->str[idx] = entry;
    return entry;
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

void tsc_pathfix(char *path) {
#ifdef _WIN32
    for(size_t i = 0; path[i] != '\0'; i++) {
        if(path[i] == '/') path[i] = '\\';
    }
#endif
    return;
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
