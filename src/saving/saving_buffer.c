#include "saving.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "../utils.h"

tsc_buffer tsc_saving_newBuffer(const char *initial) {
    tsc_buffer buffer;
    buffer.mem = tsc_strdup(initial);
    buffer.len = initial == NULL ? 0 : strlen(initial);
    buffer.cap = buffer.len;
    return buffer;
}

tsc_buffer tsc_saving_newBufferCapacity(const char *initial, size_t capacity) {
    size_t initlen = initial == NULL ? 0 : strlen(initial);
    if(capacity < initlen) {
        capacity = initlen;
    }
    tsc_buffer buffer;
    if(capacity != 0) {
        buffer.mem = malloc(sizeof(char) * (capacity + 1));
        if(initial != NULL) strcpy(buffer.mem, initial);
        else buffer.mem[0] = '\0';
        buffer.mem[capacity] = '\0';
        buffer.len = initlen;
        buffer.cap = capacity;
    } else {
        buffer.mem = NULL;
        buffer.len = 0;
        buffer.cap = 0;
    }
    return buffer;
}

void tsc_saving_deleteBuffer(tsc_buffer buffer) {
    free(buffer.mem);
}

char *tsc_saving_reserveFor(tsc_buffer *buffer, size_t amount) {
    while(buffer->len + amount > buffer->cap) {
        if(buffer->cap != 0) {
            buffer->cap *= 2;
            buffer->mem = realloc(buffer->mem, sizeof(char) * (buffer->cap + 1));
            buffer->mem[buffer->cap] = '\0';
        } else {
            buffer->cap = buffer->len + amount;
            buffer->mem = malloc(sizeof(char) * (amount + 1));
            buffer->mem[amount] = '\0';
        }
    }
    size_t idx = buffer->len;
    buffer->len += amount;
    buffer->mem[buffer->len] = '\0';
    return buffer->mem + idx;
}

void tsc_saving_write(tsc_buffer *buffer, char ch) {
    char *amount = tsc_saving_reserveFor(buffer, 1);
    *amount = ch;
}

void tsc_saving_writeStr(tsc_buffer *buffer, const char *str) {
    if(str == NULL) return;
    strcpy(tsc_saving_reserveFor(buffer, strlen(str)), str);
}

void __attribute__((format (printf, 2, 3))) tsc_saving_writeFormat(tsc_buffer *buffer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int amount = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *buf = tsc_saving_reserveFor(buffer, amount);
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
}

void tsc_saving_writeBytes(tsc_buffer *buffer, const char *mem, size_t count) {
    if(count == 0) return;
    memcpy(tsc_saving_reserveFor(buffer, count), mem, sizeof(char) * count);
}

void tsc_saving_clearBuffer(tsc_buffer *buffer) {
    buffer->len = 0;
}
