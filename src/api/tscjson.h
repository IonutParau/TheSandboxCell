#ifndef TSC_JSON_H
#define TSC_JSON_H

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

static const char *tsc_json_error[TSC_JSON_ERROR_COUNT] = {
    [TSC_JSON_ERROR_SUCCESS] = "Success",
    [TSC_JSON_PARSE_ERROR_UNTERMINATED_MULTILINE_COMMENT] = "Unterminated multiline comment starting at",
    [TSC_JSON_PARSE_ERROR_UNEXPECTED_CHARACTER] = "Unexpected character at",
    [TSC_JSON_PARSE_ERROR_INVALID_xXX_ESCAPE] = "Invalid \\xXX escape starting at",
    [TSC_JSON_PARSE_ERROR_INVALID_uXXXX_ESCAPE] = "Invalid \\uXXXX escape starting at",
    [TSC_JSON_PARSE_ERROR_INVALID_UXXXXXXXX_ESCAPE] = "Invalid \\UXXXXXXXX escape starting at",
    [TSC_JSON_PARSE_ERROR_BAD_UTF8] = "Can't parse character at",
    [TSC_JSON_PARSE_ERROR_BAD_BACKSLASH] = "Invalid '\\' escape at",
    [TSC_JSON_PARSE_ERROR_UNTERMINATED_STRING] = "Unterminated string starting at",
    [TSC_JSON_PARSE_ERROR_OBJECT_NO_KEY] = "Expected string at",
    [TSC_JSON_PARSE_ERROR_EXPECTED_COLON] = "Expected ':' at",
    [TSC_JSON_PARSE_ERROR_EXPECTED_COMMA_DELIMITER] = "Expected ',' delimiter at",
    [TSC_JSON_PARSE_ERROR_EXPECTED_ANY] = "Expecting value",
    [TSC_JSON_PARSE_ERROR_TRAILING_CHARACTERS_AFTER_VALUE] = "Trailing characters after JSON value",

    [TSC_JSON_ENCODE_ERROR_INVALID_STRING] = "Can't encode string as UTF-8",
    [TSC_JSON_ENCODE_ERROR_CANT_ENCODE_CELL] = "Unencodable value: Cell"
};

typedef struct tsc_json_error_t {
    uint32_t status;
    uint32_t index;
} tsc_json_error_t;

tsc_buffer tsc_json_encode(const tsc_value value, tsc_json_error_t *err, const int indent, const bool ensure_ascii);
tsc_value tsc_json_decode(const char *text, tsc_json_error_t *err);

#endif
