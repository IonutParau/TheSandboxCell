#ifndef TSC_SAVING_H
#define TSC_SAVING_H

#include <stddef.h>
#include "../cells/grid.h"

typedef struct tsc_saving_buffer {
    char *mem;
    size_t len;
    size_t cap;
} tsc_saving_buffer;

tsc_saving_buffer tsc_saving_newBuffer(const char *initial);
tsc_saving_buffer tsc_saving_newBufferCapacity(const char *initial, size_t capacity);
void tsc_saving_deleteBuffer(tsc_saving_buffer buffer);
void tsc_saving_write(tsc_saving_buffer *buffer, char ch);
void tsc_saving_writeStr(tsc_saving_buffer *buffer, const char *str);
void tsc_saving_writeFormat(tsc_saving_buffer *buffer, const char *fmt, ...);
void tsc_saving_writeBytes(tsc_saving_buffer *buffer, const char *mem, size_t count);

typedef int tsc_saving_encoder(tsc_saving_buffer *buffer, tsc_grid *grid);
typedef void tsc_saving_decoder(const char *code, tsc_grid *grid);

typedef struct tsc_saving_format {
    const char *name;
    const char *header;
    tsc_saving_encoder *encode;
    tsc_saving_decoder *decode;
} tsc_saving_format;

int tsc_saving_encodeWith(tsc_saving_buffer *buffer, tsc_grid *grid, const char *name);
void tsc_saving_encodeWithSmallest(tsc_saving_buffer *buffer, tsc_grid *grid);
void tsc_saving_decodeWith(const char *code, tsc_grid *grid, const char *name);
void tsc_saving_decodeWithAny(const char *code, tsc_grid *grid);

void tsc_saving_register(tsc_saving_format format);
void tsc_saving_registerCore();

#endif
