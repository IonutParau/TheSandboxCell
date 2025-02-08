#ifndef TSC_SAVING_H
#define TSC_SAVING_H

#include <stddef.h>
#include "../cells/grid.h"

typedef struct tsc_buffer {
    char *mem;
    size_t len;
    size_t cap;
} tsc_buffer;

// Legacy alias
typedef tsc_buffer tsc_saving_buffer;

tsc_buffer tsc_saving_newBuffer(const char *initial);
tsc_buffer tsc_saving_newBufferCapacity(const char *initial, size_t capacity);
void tsc_saving_deleteBuffer(tsc_buffer buffer);
char *tsc_saving_reserveFor(tsc_buffer *buffer, size_t amount);
void tsc_saving_write(tsc_buffer *buffer, char ch);
void tsc_saving_writeStr(tsc_buffer *buffer, const char *str);
void __attribute__((format (printf, 2, 3))) tsc_saving_writeFormat(tsc_buffer *buffer, const char *fmt, ...);
void tsc_saving_writeBytes(tsc_buffer *buffer, const char *mem, size_t count);
void tsc_saving_clearBuffer(tsc_buffer *buffer);

typedef int tsc_saving_encoder(tsc_buffer *buffer, tsc_grid *grid);
typedef void tsc_saving_decoder(const char *code, tsc_grid *grid);

#define TSC_SAVING_COMPATIBILITY 1

typedef struct tsc_saving_format {
    const char *name;
    const char *header;
    tsc_saving_encoder *encode;
    tsc_saving_decoder *decode;
    size_t flags;
} tsc_saving_format;

int tsc_saving_encodeWith(tsc_buffer *buffer, tsc_grid *grid, const char *name);
void tsc_saving_encodeWithSmallest(tsc_buffer *buffer, tsc_grid *grid);
void tsc_saving_decodeWith(const char *code, tsc_grid *grid, const char *name);
const char *tsc_saving_identify(const char *code);
void tsc_saving_decodeWithAny(const char *code, tsc_grid *grid);

void tsc_saving_register(tsc_saving_format format);
void tsc_saving_registerCore();

#endif
