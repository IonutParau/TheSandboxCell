#include "tscjson.h"

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

int tsc_json_strerror(void *buf, size_t capacity, tsc_json_error_t err) {
    if (err.status >= TSC_JSON_ERROR_COUNT) {
        return snprintf(buf, capacity, "Unknown error");
    }
    if (err.status <= TSC_JSON_PARSE_ERROR_TRAILING_CHARACTERS_AFTER_VALUE &&
        err.status != TSC_JSON_ERROR_SUCCESS) {
        return snprintf(buf, capacity, "%s: byte %d", tsc_json_error[err.status], err.index);
    }
    return snprintf(buf, capacity, tsc_json_error[err.status]);
}

int tsc_json_fperror(FILE* file, tsc_json_error_t err) {
    if (err.status >= TSC_JSON_ERROR_COUNT) {
        return fputs("Unknown error\n", file);
    }
    if (err.status == TSC_JSON_ERROR_SUCCESS) {
        return fputs("Success\n", file);
    }
    if (err.status <= TSC_JSON_PARSE_ERROR_TRAILING_CHARACTERS_AFTER_VALUE) {
        return fprintf(file, "JSON error: %s: byte %d\n", tsc_json_error[err.status], err.index);
    }
    return fprintf(file, "JSON error: %s\n", tsc_json_error[err.status]);
}

int tsc_json_perror(tsc_json_error_t err) {
    return tsc_json_fperror(stderr, err);
}
