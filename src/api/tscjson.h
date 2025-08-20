#ifndef TSC_JSON_H
#define TSC_JSON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // FILE*

#include "value.h"
#include "../saving/saving.h"

enum {
    TSC_JSON_ERROR_SUCCESS = 0,
    TSC_JSON_PARSE_ERROR_UNTERMINATED_MULTILINE_COMMENT,
    TSC_JSON_PARSE_ERROR_UNEXPECTED_CHARACTER,
    TSC_JSON_PARSE_ERROR_INVALID_xXX_ESCAPE,
    TSC_JSON_PARSE_ERROR_INVALID_uXXXX_ESCAPE,
    TSC_JSON_PARSE_ERROR_INVALID_UXXXXXXXX_ESCAPE,
    TSC_JSON_PARSE_ERROR_BAD_UTF8,
    TSC_JSON_PARSE_ERROR_BAD_BACKSLASH,
    TSC_JSON_PARSE_ERROR_UNTERMINATED_STRING,
    TSC_JSON_PARSE_ERROR_OBJECT_NO_KEY,
    TSC_JSON_PARSE_ERROR_EXPECTED_COLON,
    TSC_JSON_PARSE_ERROR_EXPECTED_COMMA_DELIMITER,
    TSC_JSON_PARSE_ERROR_EXPECTED_ANY,
    TSC_JSON_PARSE_ERROR_TRAILING_CHARACTERS_AFTER_VALUE,

    TSC_JSON_ENCODE_ERROR_INVALID_STRING,
    TSC_JSON_ENCODE_ERROR_CANT_ENCODE_CELL,
    TSC_JSON_ERROR_COUNT
};

typedef struct tsc_json_error_t {
    uint32_t status;
    uint32_t index;
} tsc_json_error_t;

int tsc_json_strerror(void *buf, size_t capacity, tsc_json_error_t err);
int tsc_json_fperror(FILE *file, tsc_json_error_t err);
int tsc_json_perror(tsc_json_error_t err);
tsc_buffer tsc_json_encode(const tsc_value value, tsc_json_error_t *err, const int indent, const bool ensure_ascii);
tsc_value tsc_json_decode(const char *text, tsc_json_error_t *err);

#endif
