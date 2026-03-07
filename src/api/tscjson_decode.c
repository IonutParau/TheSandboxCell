#include <stdlib.h> // fuck you floats

#include "tscjson.h"
#include "tscjson_macros.h"

// heheheha
// Yo what the freak! 2 oooooOOoOOoOOOOoOOOOoOOOOoOOoO KL[oKLP[]

#define TSC_JSON_ERROR(err, idx) ((tsc_json_error_t) { (uint32_t)(TSC_JSON_PARSE_ERROR_##err), (uint32_t)(idx) })
#define TSC_JSON_SUCCESS ((tsc_json_error_t) { (uint32_t)(TSC_JSON_ERROR_SUCCESS), 0 })

#define TSC_JSON_CALL(thing) do { \
    tsc_json_error_t result = thing; \
    if (result.status != TSC_JSON_ERROR_SUCCESS) { \
        return result; \
    } \
} while (0)

#define TSC_JSON_MEASURE_EXPECT_CHAR(expected_char, error_code) do { \
    TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx)); \
    if ((s[(*idx)++]) != expected_char) { \
        return TSC_JSON_ERROR(error_code, *idx - 1); \
    } \
} while (0)

static tsc_json_error_t tsc_json_measure_skip_whitespace(
        const char *s,
        size_t *idx
) {
    while (1) {
        unsigned char c = s[*idx];
        if (c == '\0') break;
        if (TSC_JSON_ISSPACE[c]) {
            (*idx)++;
            continue;
        }

        if (c == '/') {
            (*idx)++;
            c = s[(*idx)++];
            if (c == '/') {
                while (1) {
                    c = s[(*idx)];
                    *idx += c != '\0';
                    if (c == '\0' || c == '\n') break;
                }
                continue;
            }
            if (c == '*') {
                const size_t start = *idx;
                while (1) {
                    c = s[(*idx)++];
                    if (c == '\0') {
                        return TSC_JSON_ERROR(UNTERMINATED_MULTILINE_COMMENT, start);
                    }
                    if (c == '*' && s[*idx] == '/') {
                        (*idx)++;
                        break;
                    }
                }
                continue;
            }

            return TSC_JSON_ERROR(UNEXPECTED_CHARACTER, *idx - 2);
        }

        break;
    }
    return TSC_JSON_SUCCESS;
}

static int tsc_json_measure_number(
        const char* s,
        size_t* idx
) {
    const size_t start = *idx;
    size_t pos = start;
    int can_be_float = 1;

    if (s[pos] == '-') pos++;

    if (s[pos] == '0') {
        pos++;
        if (s[pos] == 'x') {
            can_be_float = 0;
            pos++;
            if (!TSC_JSON_ISXDIGIT[(unsigned char)s[pos]]) return 0;
            while (TSC_JSON_ISXDIGIT[(unsigned char)s[pos]]) pos++;
        }
        else if (s[pos] == 'b') {
            can_be_float = 0;
            pos++;
            if (s[pos] != '0' && s[pos] != '1') return 0;
            while (s[pos] == '0' || s[pos] == '1') pos++;
        }
        // after a leading zero we can only have '.' 'e' or end of whatever
        // because we don't have octals
        else {
            if (TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) return 0;

            if (s[pos] != '\0' &&
                s[pos] != '.' &&
                s[pos] != 'e' &&
                s[pos] != 'E' &&
                !TSC_JSON_ISSPACE[(unsigned char)s[pos]] &&
                s[pos] != ',' &&
                s[pos] != '}' &&
                s[pos] != ']'
            ) return 0;
        }
    }
    else if (TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) {
        pos++;
        while (TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) pos++;
    }
    else return 0;

    if (can_be_float) {
        if (s[pos] == '.') {
            pos++;
            if (!TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) return 0;
            while (TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) pos++;
        }

        if (s[pos] == 'e' || s[pos] == 'E') {
            pos++;
            if (s[pos] == '+' || s[pos] == '-') pos++;

            if (!TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) return 0;
            while (TSC_JSON_ISDIGIT[(unsigned char)s[pos]]) pos++;
        }
    }

    *idx = pos;
    return 1; // yes this is a valid number
}

static tsc_json_error_t tsc_json_measure_string(
        const char *s,
        size_t *idx
) {
    const size_t start = *idx - 1;

    while (1) {
        const char c = s[*idx];
        if ((unsigned char)c < ' ') {
            return TSC_JSON_ERROR(UNTERMINATED_STRING, start);
        }
        if (c == '"') {
            (*idx)++;
            break;
        }
        if (c == '\\') {
            (*idx)++;

            const char esc = s[(*idx)++];
            if (esc == '\0') {
                return TSC_JSON_ERROR(UNTERMINATED_STRING, start);
            }

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                // good
            }
            else if (esc == 'x') {
                if (!TSC_JSON_ISXDIGIT[(unsigned char)s[*idx]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 1]]) {
                        return TSC_JSON_ERROR(INVALID_xXX_ESCAPE, *idx);
                }
            }
            else if (esc == 'u') {
                if (!TSC_JSON_ISXDIGIT[(unsigned char)s[*idx]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 1]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 2]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 3]]) {
                        return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, *idx);
                }

                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 2]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 3]];

                *idx += 4;

                if (0xd800 <= cp && cp <= 0xdbff) {
                    if (s[*idx] != '\\' || s[*idx + 1] != 'u') {
                        return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, *idx);
                    }
                    *idx += 2;
                    if (!TSC_JSON_ISXDIGIT[(unsigned char)s[*idx]] ||
                        !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 1]] ||
                        !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 2]] ||
                        !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 3]]) {
                            return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, *idx);
                    }
                    const unsigned int cp2 =
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 12 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]] << 8 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 2]] << 4 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 3]];
                    if (0xdc00 <= cp2 && cp2 <= 0xdfff) {
                        *idx += 4;
                    }
                    else {
                        return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, *idx);
                    }
                }
            }
            else if (esc == 'U') {
                if (!TSC_JSON_ISXDIGIT[(unsigned char)s[*idx]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 1]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 2]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 3]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 4]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 5]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 6]] ||
                    !TSC_JSON_ISXDIGIT[(unsigned char)s[*idx + 7]]) {
                        return TSC_JSON_ERROR(INVALID_UXXXXXXXX_ESCAPE, *idx);
                }

                *idx += 8;
            }
            else {
                return TSC_JSON_ERROR(BAD_BACKSLASH, *idx);
            }
            continue;
        }

        const unsigned char b0 = (unsigned char)s[*idx];
        if ((b0 & 0x80) == 0) {
            *idx += 1;
        }
        else if ((b0 & 0xE0) == 0xC0) {
            if (s[*idx + 1] == '\0' ||
                (s[*idx + 1] & 0xC0) != 0x80 ||
                b0 < 0xC2) {
                    return TSC_JSON_ERROR(BAD_UTF8, *idx);
            }
            *idx += 2;
        }
        else if ((b0 & 0xF0) == 0xE0) {
            if (s[*idx + 1] == '\0' || s[*idx + 2] == '\0' ||
                (s[*idx + 1] & 0xC0) != 0x80 ||
                (s[*idx + 2] & 0xC0) != 0x80 ||
                (b0 == 0xE0 && (unsigned char)s[*idx + 1] < 0xA0) ||
                (b0 == 0xED && (unsigned char)s[*idx + 1] >= 0xA0)) {
                    return TSC_JSON_ERROR(BAD_UTF8, *idx);
            }
            *idx += 3;
        }
        else if ((b0 & 0xF8) == 0xF0) {
            if (s[*idx + 1] == '\0' || s[*idx + 2] == '\0' || s[*idx + 3] == '\0' ||
                (s[*idx + 1] & 0xC0) != 0x80 ||
                (s[*idx + 2] & 0xC0) != 0x80 ||
                (s[*idx + 3] & 0xC0) != 0x80 ||
                (b0 == 0xF0 && (unsigned char)s[*idx + 1] < 0x90) ||
                (b0 > 0xF4 || (b0 == 0xF4 && (unsigned char)s[*idx + 1] > 0x8F))) {
                    return TSC_JSON_ERROR(BAD_UTF8, *idx);
            }
            *idx += 4;
        }
        else {
            return TSC_JSON_ERROR(BAD_UTF8, *idx);
        }
    }

    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_measure_any(
    const char *s,
    size_t *idx,
    int *stack_size
);

static tsc_json_error_t tsc_json_measure_object(
        const char *s,
        size_t *idx,
        int *stack_size
) {
    (*stack_size)++;

    TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));

    if (s[*idx] == '}') {
        (*idx)++;
        return TSC_JSON_SUCCESS;
    }

    while (1) {
        TSC_JSON_MEASURE_EXPECT_CHAR('"', OBJECT_NO_KEY);
        TSC_JSON_CALL(tsc_json_measure_string(s, idx));
        (*stack_size)++;

        TSC_JSON_MEASURE_EXPECT_CHAR(':', EXPECTED_COLON);

        TSC_JSON_CALL(tsc_json_measure_any(s, idx, stack_size));
        TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));

        const char sep = s[(*idx)++];
        if (sep == '}') return TSC_JSON_SUCCESS;
        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));
            if (s[*idx] == '}') {
                (*idx)++;
                return TSC_JSON_SUCCESS;
            }
            continue;
        }
        return TSC_JSON_ERROR(EXPECTED_COMMA_DELIMITER, *idx - 1);
    }
}

static tsc_json_error_t tsc_json_measure_array(
        const char *s,
        size_t *idx,
        int *stack_size
) {
    (*stack_size)++;

    TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));

    if (s[*idx] == ']') {
        (*idx)++;
        return TSC_JSON_SUCCESS;
    }

    while (1) {
        TSC_JSON_CALL(tsc_json_measure_any(s, idx, stack_size));
        TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));

        const char sep = s[(*idx)++];
        if (sep == ']') return TSC_JSON_SUCCESS;

        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));
            if (s[*idx] == ']') {
                (*idx)++;
                return TSC_JSON_SUCCESS;
            }
            continue;
        }
        return TSC_JSON_ERROR(EXPECTED_COMMA_DELIMITER, *idx - 1);
    }
}

static tsc_json_error_t tsc_json_measure_any(
        const char *s,
        size_t *idx,
        int *stack_size
) {
    TSC_JSON_CALL(tsc_json_measure_skip_whitespace(s, idx));
    const char c = s[(*idx)++];

    if (c == '{') {
        TSC_JSON_CALL(tsc_json_measure_object(s, idx, stack_size));
        return TSC_JSON_SUCCESS;
    }

    if (c == '[') {
        TSC_JSON_CALL(tsc_json_measure_array(s, idx, stack_size));
        return TSC_JSON_SUCCESS;
    }

    if (c == '"') {
        TSC_JSON_CALL(tsc_json_measure_string(s, idx));
        (*stack_size)++;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           't' &&
        s[*idx]     == 'r' &&
        s[*idx + 1] == 'u' &&
        s[*idx + 2] == 'e'
    ) {
        *idx += 3;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'f' &&
        s[*idx]     == 'a' &&
        s[*idx + 1] == 'l' &&
        s[*idx + 2] == 's' &&
        s[*idx + 3] == 'e'
    ) {
        *idx += 4;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'n' &&
        s[*idx]     == 'u' &&
        s[*idx + 1] == 'l' &&
        s[*idx + 2] == 'l'
    ) {
        *idx += 3;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'N' &&
        s[*idx]     == 'a' &&
        s[*idx + 1] == 'N'
    ) {
        *idx += 2;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'I' &&
        s[*idx]     == 'n' &&
        s[*idx + 1] == 'f' &&
        s[*idx + 2] == 'i' &&
        s[*idx + 3] == 'n' &&
        s[*idx + 4] == 'i' &&
        s[*idx + 5] == 't' &&
        s[*idx + 6] == 'y'
    ) {
        *idx += 7;
        return TSC_JSON_SUCCESS;
    }

    if (c ==           '-' &&
        s[*idx]     == 'I' &&
        s[*idx + 1] == 'n' &&
        s[*idx + 2] == 'f' &&
        s[*idx + 3] == 'i' &&
        s[*idx + 4] == 'n' &&
        s[*idx + 5] == 'i' &&
        s[*idx + 6] == 't' &&
        s[*idx + 7] == 'y'
    ) {
        *idx += 8;
        return TSC_JSON_SUCCESS;
    }

    (*idx)--;
    if (tsc_json_measure_number(s, idx)) {
        (*stack_size)++;
        return TSC_JSON_SUCCESS;
    }
    return TSC_JSON_ERROR(EXPECTED_ANY, *idx);
}

static const char *tsc_json_stack_skip_whitespace(const char *s) {
    while (1) {
        unsigned char c = *s;
        if (c == '\0') break;
        if (TSC_JSON_ISSPACE[c]) {
            s++;
            continue;
        }

        if (c == '/') {
            s++;
            c = *s;
            if (c == '/') {
                while (1) {
                    c = *s;
                    s += c != '\0';
                    if (c == '\0' || c == '\n') break;
                }
                continue;
            }
            if (c == '*') {
                while (1) {
                    c = *s++;
                    if (c == '*' && *s == '/') {
                        s++;
                        break;
                    }
                }
                continue;
            }
        }
        break;
    }
    return s;
}

// Number format:
// T - type int:00 hex:01 bin:10 float:11
// s - string size
// TT ss ss ss  ss ss ss ss
// 10 00 00 00  11 10 10 10
static const char *tsc_json_stack_number(const char *s, tscjson_stack_t *stack) {
    tscjson_stack_t own_stack = *stack;
    (*stack)++;

    const char *start = s;
    int type = 0;

    if (*s == '-') s++;

    if (*s == '0') {
        s++;
        if (*s == 'x') {
            type = 1;
            s++;
            while (TSC_JSON_ISXDIGIT[(unsigned char)*s]) s++;
        }
        else if (*s == 'b') {
            type = 2;
            s++;
            while (*s == '0' || *s == '1') s++;
        }
    }
    else if (TSC_JSON_ISDIGIT[(unsigned char)*s]) {
        s++;
        while (TSC_JSON_ISDIGIT[(unsigned char)*s]) s++;
    }

    if (type == 0) {
        if (*s == '.') {
            type = 3;
            s++;
            while (TSC_JSON_ISDIGIT[(unsigned char)*s]) s++;
        }

        if (*s == 'e' || *s == 'E') {
            type = 3;
            s++;
            if (*s == '+' || *s == '-') s++;
            while (TSC_JSON_ISDIGIT[(unsigned char)*s]) s++;
        }
    }

    static const uint64_t thing[8] = {0x3F, 0x3FFF, 0x3FFFFF, 0x3FFFFFFF, 0x3FFFFFFFFF, 0x3FFFFFFFFFFF, 0x3FFFFFFFFFFFFF, 0x3FFFFFFFFFFFFF};
    *own_stack = (tscjson_stack_type_t)(type            << (sizeof(tscjson_stack_type_t)*8-2)) |
                 (tscjson_stack_type_t)((s - start) & thing[sizeof(tscjson_stack_type_t) - 1]);
    return s;
}

static const char *tsc_json_stack_string(const char *s, tscjson_stack_t *stack) {
    tscjson_stack_t own_stack = *stack;
    (*stack)++;
    (*own_stack)++; // \0!!!!!!!

    while (1) {
        const char c = *s;
        if (c == '"') {
            s++;
            break;
        }
        if (c == '\\') {
            s++;
            const char esc = *s++;

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                (*own_stack)++;
            }
            else if (esc == 'x') {
                s += 2;
                *own_stack += 1;
            }
            else if (esc == 'u') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[0]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[1]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[2]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[3]];

                s += 4;

                if (0xd800 <= cp && cp <= 0xdbff) {
                    s += 2;
                    const unsigned int cp2 =
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[0]] << 12 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[1]] << 8 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[2]] << 4 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[3]];

                    if (0xdc00 <= cp2 && cp2 <= 0xdfff) {
                        s += 4;
                        *own_stack += 4;
                    }
				}
                else {
                    if (cp <= 0x7F) {
                        *own_stack += 1;
                    }
                    else if (cp <= 0x7FF) {
                        *own_stack += 2;
                    }
                    else {
                        *own_stack += 3;
                    }
                }
            }
            else if (esc == 'U') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[0]] << 28 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[1]] << 24 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[2]] << 20 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[3]] << 16 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[4]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[5]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[6]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[7]];

                s += 8;

                if (cp <= 0x7F) {
                    *own_stack += 1;
                }
                else if (cp <= 0x7FF) {
                    *own_stack += 2;
                }
                else if (cp <= 0xFFFF) {
                    *own_stack += 3;
                }
                else {
                    *own_stack += 4;
                }
            }
            continue;
        }

        const unsigned char b0 = (unsigned char)*s;

        if ((b0 & 0x80) == 0) {
            s++;
            *own_stack += 1;
        }
        else if ((b0 & 0xE0) == 0xC0) {
            s += 2;
            *own_stack += 2;
        }
        else if ((b0 & 0xF0) == 0xE0) {
            s += 3;
            *own_stack += 3;
        }
        else if ((b0 & 0xF8) == 0xF0) {
            s += 4;
            *own_stack += 4;
        }
    }

    return s;
}

static const char *tsc_json_stack_any(const char *s, tscjson_stack_t *stack);

static const char *tsc_json_stack_object(const char *s, tscjson_stack_t *stack) {
    tscjson_stack_t own_stack = *stack;
    (*stack)++;

    s = tsc_json_stack_skip_whitespace(s);

    if (*s == '}') {
        s++;
        return s;
    }

    while (1) {
        (*own_stack)++;

        s = tsc_json_stack_skip_whitespace(s);
        // Next char is '"'
        s++;
        s = tsc_json_stack_string(s, stack);

        s = tsc_json_stack_skip_whitespace(s);
        // Next char is ':'
        s++;
        s = tsc_json_stack_any(s, stack);
        s = tsc_json_stack_skip_whitespace(s);

        const char sep = *s++;
        if (sep == '}') return s;
        if (sep == ',') {
            s = tsc_json_stack_skip_whitespace(s);
            if (*s == '}') {
                s++;
                return s;
            }
        }
    }
}

static const char *tsc_json_stack_array(const char *s, tscjson_stack_t *stack) {
    tscjson_stack_t own_stack = *stack;
    (*stack)++;

    s = tsc_json_stack_skip_whitespace(s);

    if (*s == ']') {
        s++;
        return s;
    }

    while (1) {
        (*own_stack)++;
        s = tsc_json_stack_any(s, stack);
        s = tsc_json_stack_skip_whitespace(s);

        const char sep = *s++;
        if (sep == ']') return s;

        if (sep == ',') {
            s = tsc_json_stack_skip_whitespace(s);
            if (*s == ']') {
                s++;
                return s;
            }
        }
    }
}

static const char *tsc_json_stack_any(const char* s, tscjson_stack_t *stack) {
    s = tsc_json_stack_skip_whitespace(s);
    if (*s == '{') return tsc_json_stack_object(s + 1, stack);
    if (*s == '[') return tsc_json_stack_array(s + 1, stack);
    if (*s == '"') return tsc_json_stack_string(s + 1, stack);
    if (*s == 't') return s + 4;
    if (*s == 'f') return s + 5;
    if (*s == 'n') return s + 4;
    if (*s == 'N') return s + 3;
    if (*s == 'I') return s + 8;
    if (*s == '-' && s[1] == 'I') return s + 9;
    return tsc_json_stack_number(s, stack);
}

static tsc_value tsc_json_decode_number(const char **s, tscjson_stack_t *stack) {
    const tscjson_stack_type_t packed = **stack;
    (*stack)++;

    const char *start = *s;
    static const uint64_t thing[8] = {0x3F, 0x3FFF, 0x3FFFFF, 0x3FFFFFFF, 0x3FFFFFFFFF, 0x3FFFFFFFFFFF, 0x3FFFFFFFFFFFFF, 0x3FFFFFFFFFFFFF};
    const uint32_t type = (packed >> (sizeof(tscjson_stack_type_t)*8-2)) & 3;
    const uint32_t size = packed & thing[sizeof(tscjson_stack_type_t) - 1];

    *s += size;

    size_t i = 0;
    const int sign = start[i] == '-' ? -1 : 1;
    if (start[i] == '-') i++;

    // TODO: make it not use glibc
    // horrible idea i spent a few hours and failed miserably like wtf man
    if (type == 3) return (tsc_value) { .tag = TSC_VALUE_NUMBER, .number = sign * strtod(start + i, 0) };

    if (type == 2) {
        i += 2;
        int64_t result = 0;
        while (start[i] == '0' || start[i] == '1') {
            result *= 2;
            result += start[i] - '0';
            i++;
        }
        return (tsc_value){ .tag = TSC_VALUE_INT, .integer = result * sign };
    }

    if (type == 1) {
        i += 2;
        int64_t result = 0;
        while (TSC_JSON_ISXDIGIT[(unsigned char)start[i]]) {
            result *= 16;
            result += TSC_JSON_XDIGIT[(unsigned char)start[i]];
            i++;
        }
        return (tsc_value){ .tag = TSC_VALUE_INT, .integer = result * sign };
    }

    {
        int64_t result = 0;
        while (TSC_JSON_ISDIGIT[(unsigned char)start[i]]) {
            result *= 10;
            result += start[i] - '0';
            i++;
        }
        return (tsc_value){ .tag = TSC_VALUE_INT, .integer = result * sign };
    }
}

static int tsc_json_utf8_encode(const unsigned int cp, char *p) {
    if (cp <= 0x7F) {
        p[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        p[0] = (char)(0xC0 | cp >> 6);
        p[1] = (char)(0x80 | cp & 0x3F);
        return 2;
    }
    if (cp <= 0xFFFF) {
        p[0] = (char)(0xE0 | cp >> 12);
        p[1] = (char)(0x80 | cp >> 6 & 0x3F);
        p[2] = (char)(0x80 | cp & 0x3F);
        return 3;
    }
    p[0] = (char)(0xF0 | cp >> 18);
    p[1] = (char)(0x80 | cp >> 12 & 0x3F);
    p[2] = (char)(0x80 | cp >> 6 & 0x3F);
    p[3] = (char)(0x80 | cp & 0x3F);
    return 4;
}

static char* tsc_json_decode_string(const char **s, tscjson_stack_t *stack, size_t *size) {
    const tscjson_stack_type_t len = **stack;
    (*stack)++;
    if (size != NULL) *size = len;
    char* b = malloc(len);
    char* b_begin = b;

    while (**s != '"') {
        if (**s == '\\') {
            (*s)++;
            const char esc = *(*s)++;

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                *b++ = TSC_JSON_BACKSLASH[(unsigned char)esc];
            }
            else if (esc == 'x') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[0]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[1]];
                *s += 2;
                b += tsc_json_utf8_encode(cp, b);
            }
            else if (esc == 'u') {
                unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[0]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[1]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[2]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[3]];
                *s += 4;

                if (0xd800 <= cp && cp <= 0xdbff) {
                    const unsigned int cp2 =
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[2]] << 12 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[3]] << 8 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[4]] << 4 |
                        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[5]];

                    cp = 0x10000 + (((cp - 0xd800) << 10) | (cp2 - 0xdc00));
                    *s += 6;
                }
                b += tsc_json_utf8_encode(cp, b);
            }
            else if (esc == 'U') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[0]] << 28 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[1]] << 24 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[2]] << 20 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[3]] << 16 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[4]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[5]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[6]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)(*s)[7]];
                *s += 8;
                b += tsc_json_utf8_encode(cp, b);
            }
        }
        else {
            const unsigned char b0 = (unsigned char)(**s);

            if ((b0 & 0x80) == 0) {
                *b++ = (*s)[0];
                (*s)++;
            }
            else if ((b0 & 0xE0) == 0xC0) {
                *b++ = (*s)[0];
                *b++ = (*s)[1];
                *s += 2;
            }
            else if ((b0 & 0xF0) == 0xE0) {
                *b++ = (*s)[0];
                *b++ = (*s)[1];
                *b++ = (*s)[2];
                *s += 3;
            }
            else if ((b0 & 0xF8) == 0xF0) {
                *b++ = (*s)[0];
                *b++ = (*s)[1];
                *b++ = (*s)[2];
                *b++ = (*s)[3];
                *s += 4;
            }
        }
    }

    (*s)++;
    *b = '\0';

    return b_begin;
}

static tsc_value tsc_json_decode_any(const char **s, tscjson_stack_t *stack);

static tsc_value tsc_json_decode_object(const char **s, tscjson_stack_t *stack) {
    const tscjson_stack_type_t len = **stack;
    (*stack)++;
    tsc_value v;
    v.tag = TSC_VALUE_OBJECT;
    v.object = malloc(sizeof(tsc_object_t));
    v.object->refc = 1;
    v.object->len = len;
    v.object->keys = len == 0 ? NULL : malloc(sizeof(char *) * len);
    v.object->values = len == 0 ? NULL : malloc(sizeof(tsc_value) * len);

    *s = tsc_json_stack_skip_whitespace(*s);

    if (**s == '}') {
        (*s)++;
        return v;
    }

    for (int i = 0;; i++) {
        *s = tsc_json_stack_skip_whitespace(*s);
        // Next char is '"'
        (*s)++;
        v.object->keys[i] = tsc_json_decode_string(s, stack, NULL);
        *s = tsc_json_stack_skip_whitespace(*s);
        // Next char is ':'
        (*s)++;
        v.object->values[i] = tsc_json_decode_any(s, stack);

        *s = tsc_json_stack_skip_whitespace(*s);

        const char sep = *(*s)++;
        if (sep == '}') return v;
        if (sep == ',') {
            *s = tsc_json_stack_skip_whitespace(*s);
            if (**s == '}') {
                (*s)++;
                return v;
            }
        }
    }
}

static tsc_value tsc_json_decode_array(const char **s, tscjson_stack_t *stack) {
    const tscjson_stack_type_t len = **stack;
    (*stack)++;
    tsc_value v;
    v.tag = TSC_VALUE_ARRAY;
    v.array = malloc(sizeof(tsc_array_t));
    v.array->refc = 1;
    v.array->valuec = len;
    v.array->values = len == 0 ? NULL : malloc(sizeof(tsc_value) * len);

    *s = tsc_json_stack_skip_whitespace(*s);

    if (**s == ']') {
        (*s)++;
        return v;
    }

    for (int i = 0;; i++) {
        v.array->values[i] = tsc_json_decode_any(s, stack);
        *s = tsc_json_stack_skip_whitespace(*s);

        const char sep = *(*s)++;
        if (sep == ']') return v;

        if (sep == ',') {
            *s = tsc_json_stack_skip_whitespace(*s);
            if (**s == ']') {
                (*s)++;
                return v;
            }
        }
    }
}

static tsc_value tsc_json_decode_any(const char **s, tscjson_stack_t *stack) {
    *s = tsc_json_stack_skip_whitespace(*s);
    if (**s == '{') {
        *s += 1;
        return tsc_json_decode_object(s, stack);
    }
    if (**s == '[') {
        *s += 1;
        return tsc_json_decode_array(s, stack);
    }
    if (**s == '"') {
        *s += 1;
        tsc_string_t *string = malloc(sizeof(tsc_string_t));
        string->refc = 1;
        string->memory = tsc_json_decode_string(s, stack, &string->len);
        return (tsc_value){.tag = TSC_VALUE_STRING, {.string = string}};
    }

    if (**s == 't') {
        *s += 4;
        return (tsc_value){.tag = TSC_VALUE_BOOL, {.boolean = 1}};
    }
    if (**s == 'f') {
        *s += 5;
        return (tsc_value){.tag = TSC_VALUE_BOOL, {.boolean = 0}};
    }
    if (**s == 'n') {
        *s += 4;
        return (tsc_value){.tag = TSC_VALUE_NULL};
    }
    // if this does not work your compiler is a lame sucker
    if (**s == 'N') {
        *s += 3;
        return (tsc_value){.tag = TSC_VALUE_NUMBER, {.number = 0.0/0.0}};
    }
    if (**s == 'I') {
        *s += 7;
        return (tsc_value){.tag = TSC_VALUE_NUMBER, {.number = 1.0/0.0}};
    }
    if (**s == '-' && (*s)[1] == 'I') {
        *s += 8;
        return (tsc_value){.tag = TSC_VALUE_NUMBER, {.number = -1.0/0.0}};
    }
    return tsc_json_decode_number(s, stack);
}

tsc_value tsc_json_decode(
        const char *text,
        tsc_json_error_t *err
) {
    size_t idx = 0;

    int stack_size = 0;
    tsc_json_error_t result = tsc_json_measure_any(text, &idx, &stack_size);
    if (err != NULL) *err = result;
    if (result.status != TSC_JSON_ERROR_SUCCESS) {
        return tsc_null();
    }
    result = tsc_json_measure_skip_whitespace(text, &idx);
    if (err != NULL) *err = result;
    if (result.status != TSC_JSON_ERROR_SUCCESS) {
        return tsc_null();
    }
    if (text[idx] != '\0') {
        if (err != NULL) *err = TSC_JSON_ERROR(TRAILING_CHARACTERS_AFTER_VALUE, idx);
        return tsc_null();
    }

    tscjson_stack_t stack, stack_begin;
    if (stack_size != 0) {
        TSC_JSON_ALLOC_STACK(&stack, stack_size * sizeof(stack[0]));
        stack_begin = stack;
        TSC_JSON_BZERO_STACK(&stack_begin, stack_size * sizeof(stack[0]));
        tsc_json_stack_any(text, &stack);
        stack = stack_begin;
    }

    const tsc_value val = tsc_json_decode_any(&text, &stack);
    if (stack_size != 0) TSC_JSON_FREE_STACK(&stack_begin);
    if (err != NULL) *err = TSC_JSON_SUCCESS;
    return val;
}
