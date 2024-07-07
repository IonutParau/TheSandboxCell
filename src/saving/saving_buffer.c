#include "saving.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "../utils.h"

tsc_saving_buffer tsc_saving_newBuffer(const char *initial) {
    tsc_saving_buffer buffer;
    buffer.mem = tsc_strdup(initial);
    buffer.len = initial == NULL ? 0 : strlen(initial);
    return buffer;
}

void tsc_saving_deleteBuffer(tsc_saving_buffer buffer) {
    free(buffer.mem);
}

void tsc_saving_write(tsc_saving_buffer *buffer, char ch) {
    size_t idx = buffer->len++;
    buffer->mem = realloc(buffer->mem, sizeof(char) * (buffer->len + 1));
    buffer->mem[idx] = ch;
    buffer->mem[buffer->len] = '\0';
}

static char *tsc_saving_reserveFor(tsc_saving_buffer *buffer, size_t amount) {
    size_t idx = buffer->len;
    buffer->len += amount;
    buffer->mem = realloc(buffer->mem, sizeof(char) * (buffer->len + 1));
    buffer->mem[buffer->len] = '\0';
    return buffer->mem + idx;
}

void tsc_saving_writeStr(tsc_saving_buffer *buffer, const char *str) {
   strcpy(tsc_saving_reserveFor(buffer, strlen(str)), str); 
}

void tsc_saving_writeFormat(tsc_saving_buffer *buffer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int amount = vsnprintf(NULL, 0, fmt, args);
    char *buf = tsc_saving_reserveFor(buffer, amount);
    vsprintf(buf, fmt, args);
    va_end(args);
}

void tsc_saving_writeBytes(tsc_saving_buffer *buffer, const char *mem, size_t count) {
    memcpy(tsc_saving_reserveFor(buffer, count), mem, sizeof(char) * count);
}
