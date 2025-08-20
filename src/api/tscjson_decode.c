#include <stdlib.h> // fuck you floats
                    // also we need it for string allocations

#include "tscjson.h"
#include "tscjson_macros.h"

#define TSC_JSON_CALL(thing, another_thing) do { \
    tsc_json_error_t result = thing; \
    if (result.status != TSC_JSON_ERROR_SUCCESS) { \
        another_thing; \
        return result; \
    } \
} while (0)

#define TSC_JSON_ERROR(err, idx) ((tsc_json_error_t) { (uint32_t)(TSC_JSON_PARSE_ERROR_##err), (uint32_t)(idx) })
#define TSC_JSON_SUCCESS ((tsc_json_error_t) { (uint32_t)(TSC_JSON_ERROR_SUCCESS), 0 })
#define TSC_JSON_EXPECT_CHAR(expected_char, error_code, thing) do { \
    TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), thing); \
    if ((s[(*idx)++]) != expected_char) { \
        thing; \
        return TSC_JSON_ERROR(error_code, *idx - 1); \
    } \
} while (0)

static void tsc_json_skip_line_comment(
        const char *s,
        size_t *idx
) {
    while (1) {
        const unsigned char c = s[(*idx)];
        *idx += c != '\0';
        if (c == '\0' || c == '\n') break;
    }
}

static tsc_json_error_t tsc_json_skip_block_comment(
        const char *s,
        size_t *idx
) {
    const size_t start = *idx;
    while (1) {
        const char c = s[(*idx)++];
        if (c == '\0') {
            return TSC_JSON_ERROR(UNTERMINATED_MULTILINE_COMMENT, start);
        }
        if (c == '*' && s[*idx] == '/') {
            (*idx)++;
            return TSC_JSON_SUCCESS;
        }
    }
}

static tsc_json_error_t tsc_json_skip_whitespace(
        const char *s,
        size_t *idx
) {
    while (1) {
        char c = s[*idx];
        if (c == '\0') break;
        if (isspace(c)) {
            (*idx)++;
            continue;
        }

        if (c == '/') {
            (*idx)++;
            c = s[(*idx)++];
            if (c == '/') {
                tsc_json_skip_line_comment(s, idx);
                continue;
            }
            if (c == '*') {
                TSC_JSON_CALL(tsc_json_skip_block_comment(s, idx), ;);
                continue;
            }

            return TSC_JSON_ERROR(UNEXPECTED_CHARACTER, *idx - 2);
        }

        break;
    }
    return TSC_JSON_SUCCESS;
}

static int tsc_json_utf8_encode(
        const unsigned int cp,
        char *p
) {
    if (cp <= 0x7F) {
        p[0] = cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        p[0] = 0xC0 | cp >> 6;
        p[1] = 0x80 | cp & 0x3F;
        return 2;
    }
    if (cp <= 0xFFFF) {
        p[0] = 0xE0 | cp >> 12;
        p[1] = 0x80 | cp >> 6 & 0x3F;
        p[2] = 0x80 | cp & 0x3F;
        return 3;
    }
    p[0] = 0xF0 | cp >> 18;
    p[1] = 0x80 | cp >> 12 & 0x3F;
    p[2] = 0x80 | cp >> 6 & 0x3F;
    p[3] = 0x80 | cp & 0x3F;
    return 4;
}

static tsc_json_error_t tsc_json_calculate_string_length(
        const char *s,
        size_t start_idx,
        size_t *out_length
) {
    size_t length = 1;
    size_t temp_idx = start_idx;

    while (1) {
        const char c = s[temp_idx];
        if ((unsigned char)c < ' ') {
            return TSC_JSON_ERROR(UNTERMINATED_STRING, start_idx - 1);
        }
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            temp_idx++;
            const char esc = s[temp_idx];
            if (esc == '\0') {
                return TSC_JSON_ERROR(UNTERMINATED_STRING, start_idx - 1);
            }
            temp_idx++;

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                length++;
            }
            else if (esc == 'x') {
                if (!isxdigit(s[temp_idx]) || !isxdigit(s[temp_idx + 1])) {
                    return TSC_JSON_ERROR(INVALID_xXX_ESCAPE, temp_idx);
                }
                temp_idx += 2;
                length += 1;
            }
            else if (esc == 'u') {
                if (!isxdigit(s[temp_idx]) || !isxdigit(s[temp_idx + 1]) ||
                    !isxdigit(s[temp_idx + 2]) || !isxdigit(s[temp_idx + 3])) {
                    return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, temp_idx);
                }

                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 1]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 2]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 3]];

                temp_idx += 4;

                if (cp <= 0x7F) {
                    length += 1;
                }
                else if (cp <= 0x7FF) {
                    length += 2;
                }
                else {
                    length += 3;
                }
            }
            else if (esc == 'U') {
                if (!isxdigit(s[temp_idx]) || !isxdigit(s[temp_idx + 1]) ||
                    !isxdigit(s[temp_idx + 2]) || !isxdigit(s[temp_idx + 3]) ||
                    !isxdigit(s[temp_idx + 4]) || !isxdigit(s[temp_idx + 5]) ||
                    !isxdigit(s[temp_idx + 6]) || !isxdigit(s[temp_idx + 7])) {
                    return TSC_JSON_ERROR(INVALID_UXXXXXXXX_ESCAPE, temp_idx);
                }

                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx]] << 28 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 1]] << 24 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 2]] << 20 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 3]] << 16 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 4]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 5]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 6]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[temp_idx + 7]];

                temp_idx += 8;

                if (cp <= 0x7F) {
                    length += 1;
                }
                else if (cp <= 0x7FF) {
                    length += 2;
                }
                else if (cp <= 0xFFFF) {
                    length += 3;
                }
                else {
                    length += 4;
                }
            }
            else {
                return TSC_JSON_ERROR(BAD_BACKSLASH, temp_idx);
            }
            continue;
        }

        const unsigned char b0 = (unsigned char)s[temp_idx];

        if ((b0 & 0x80) == 0) {
            temp_idx++;
            length += 1;
        }
        else if ((b0 & 0xE0) == 0xC0) {
            if (s[temp_idx + 1] == '\0' ||
                (s[temp_idx + 1] & 0xC0) != 0x80 ||
                b0 < 0xC2) {
                return TSC_JSON_ERROR(BAD_UTF8, temp_idx);
            }
            temp_idx += 2;
            length += 2;
        }
        else if ((b0 & 0xF0) == 0xE0) {
            if (s[temp_idx + 1] == '\0' || s[temp_idx + 2] == '\0' ||
                (s[temp_idx + 1] & 0xC0) != 0x80 ||
                (s[temp_idx + 2] & 0xC0) != 0x80 ||
                (b0 == 0xE0 && (unsigned char)s[temp_idx + 1] < 0xA0) ||
                (b0 == 0xED && (unsigned char)s[temp_idx + 1] >= 0xA0)) {
                return TSC_JSON_ERROR(BAD_UTF8, temp_idx);
            }
            temp_idx += 3;
            length += 3;
        }
        else if ((b0 & 0xF8) == 0xF0) {
            if (s[temp_idx + 1] == '\0' || s[temp_idx + 2] == '\0' || s[temp_idx + 3] == '\0' ||
                (s[temp_idx + 1] & 0xC0) != 0x80 ||
                (s[temp_idx + 2] & 0xC0) != 0x80 ||
                (s[temp_idx + 3] & 0xC0) != 0x80 ||
                (b0 == 0xF0 && (unsigned char)s[temp_idx + 1] < 0x90) ||
                (b0 > 0xF4 || (b0 == 0xF4 && (unsigned char)s[temp_idx + 1] > 0x8F))) {
                return TSC_JSON_ERROR(BAD_UTF8, temp_idx);
            }
            temp_idx += 4;
            length += 4;
        }
        else {
            return TSC_JSON_ERROR(BAD_UTF8, temp_idx);
        }
    }

    *out_length = length;
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_decode_string(
        const char *s,
        size_t *idx,
        char **out
) {
    size_t needed_length;
    TSC_JSON_CALL(tsc_json_calculate_string_length(s, *idx, &needed_length), ;);
    char *buf = malloc(needed_length);
    size_t buf_idx = 0;

    while (s[*idx] != '"') {
        const char c = s[*idx];

        if (c == '\\') {
            (*idx)++;
            const char esc = s[*idx];
            (*idx)++;

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                buf[buf_idx++] = TSC_JSON_BACKSLASH[(unsigned char)esc];
            }
            else if (esc == 'x') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]];
                *idx += 2;
                buf_idx += tsc_json_utf8_encode(cp, buf + buf_idx); // Pass the correct buffer offset
            }
            else if (esc == 'u') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 2]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 3]];
                *idx += 4;
                buf_idx += tsc_json_utf8_encode(cp, buf + buf_idx); // Pass the correct buffer offset
            }
            else if (esc == 'U') {
                const unsigned int cp =
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 28 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]] << 24 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 2]] << 20 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 3]] << 16 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 4]] << 12 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 5]] << 8 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 6]] << 4 |
                    (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 7]];
                *idx += 8;
                buf_idx += tsc_json_utf8_encode(cp, buf + buf_idx); // Pass the correct buffer offset
            }
        }
        else {
            const unsigned char b0 = (unsigned char)s[*idx];

            if ((b0 & 0x80) == 0) {
                buf[buf_idx++] = s[(*idx)++];
            }
            else if ((b0 & 0xE0) == 0xC0) {
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
            }
            else if ((b0 & 0xF0) == 0xE0) {
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
            }
            else if ((b0 & 0xF8) == 0xF0) {
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
                buf[buf_idx++] = s[(*idx)++];
            }
        }
    }

    (*idx)++;
    buf[buf_idx] = '\0';

    *out = buf;
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_decode_any(
    const char *s,
    size_t *idx,
    tsc_value *out
);

static tsc_json_error_t tsc_json_decode_object(
        const char *s,
        size_t *idx,
        tsc_value *out
) {
    *out = tsc_object();

    TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));

    if (s[*idx] == '}') {
        (*idx)++;
        return TSC_JSON_SUCCESS;
    }

    while (1) {
        TSC_JSON_EXPECT_CHAR('"', OBJECT_NO_KEY, tsc_destroy(*out));
        char* key;
        TSC_JSON_CALL(tsc_json_decode_string(s, idx, &key), tsc_destroy(*out));

        TSC_JSON_EXPECT_CHAR(':', EXPECTED_COLON, tsc_destroy(*out));

        tsc_value value;
        TSC_JSON_CALL(tsc_json_decode_any(s, idx, &value), tsc_destroy(*out));
        tsc_setKey(*out, key, value);
        free(key);
        tsc_destroy(value);

        TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));

        const char sep = s[(*idx)++];
        if (sep == '}') return TSC_JSON_SUCCESS;
        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));
            if (s[*idx] == '}') {
                (*idx)++;
                return TSC_JSON_SUCCESS;
            }
            continue;
        }
        return TSC_JSON_ERROR(EXPECTED_COMMA_DELIMITER, *idx - 1);
    }
}

static tsc_json_error_t tsc_json_decode_array(
        const char *s,
        size_t *idx,
        tsc_value *out
) {
    *out = tsc_array(0);

    TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));

    if (s[*idx] == ']') {
        (*idx)++;
        return TSC_JSON_SUCCESS;
    }

    while (1) {
        tsc_value value;
        TSC_JSON_CALL(tsc_json_decode_any(s, idx, &value), tsc_destroy(*out));
        tsc_append(*out, value);
        tsc_destroy(value);

        TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));

        const char sep = s[(*idx)++];
        if (sep == ']') return TSC_JSON_SUCCESS;

        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));
            if (s[*idx] == ']') {
                (*idx)++;
                return TSC_JSON_SUCCESS;
            }
            continue;
        }
        return TSC_JSON_ERROR(EXPECTED_COMMA_DELIMITER, *idx - 1);
    }
}

// oh shit
static int tsc_json_decode_number(
        const char* s,
        size_t* idx,
        tsc_value* out
) {
    const size_t start = *idx;
    size_t pos = start;
    int is_float = 0;
    int is_hex = 0;
    int is_bin = 0;

    if (s[pos] == '-') pos++;

    if (s[pos] == '0') {
        pos++;
        if (s[pos] == 'x') {
            is_hex = 1;
            pos++;
            if (!isxdigit(s[pos])) return 0;
            while (isxdigit(s[pos])) pos++;
        }
        else if (s[pos] == 'b') {
            is_bin = 1;
            pos++;
            if (s[pos] != '0' && s[pos] != '1') return 0;
            while (s[pos] == '0' || s[pos] == '1') pos++;
        }
        // after a leading zero we can only have '.' 'e' or end of whatever
        // because we don't have octals
        else {
            if (isdigit(s[pos])) return 0;

            if (s[pos] != '\0' &&
                s[pos] != '.' &&
                s[pos] != 'e' &&
                s[pos] != 'E' &&
                !isspace(s[pos]) &&
                s[pos] != ',' &&
                s[pos] != '}' &&
                s[pos] != ']'
            ) return 0;
        }
    }
    else if (isdigit(s[pos])) {
        pos++;
        while (isdigit(s[pos])) pos++;
    }
    else return 0;

    if (!is_hex && !is_bin) {
        if (s[pos] == '.') {
            is_float = 1;
            pos++;
            if (!isdigit(s[pos])) return 0;
            while (isdigit(s[pos])) pos++;
        }

        if (s[pos] == 'e' || s[pos] == 'E') {
            is_float = 1;
            pos++;
            if (s[pos] == '+' || s[pos] == '-') pos++;

            if (!isdigit(s[pos])) return 0;
            while (isdigit(s[pos])) pos++;
        }
    }

    *idx = pos;

    if (is_float) {
        // TODO: make it not use FPU and glibc
        *out = tsc_number(atof(s + start));
    }
    else if (is_bin) {
        int64_t result = 0;
        size_t i = start;

        const int sign = s[i] == '-' ? -1 : 1;
        if (s[i] == '-') i++;

        i += 2;

        while (i < pos && (s[i] == '0' || s[i] == '1')) {
            if (result > INT64_MAX / 2 || (result == INT64_MAX / 2 && s[i] - '0' > INT64_MAX % 2)) {
                double dresult = (double)result;
                while (i < pos && (s[i] == '0' || s[i] == '1')) {
                    dresult = dresult * 2.0 + (s[i] - '0');
                    i++;
                }
                *out = tsc_number(dresult * sign);
                return 0;
            }
            result = result * 2 + (s[i] - '0');
            i++;
        }

        *out = tsc_int(result * sign);
    }
    else if (is_hex) {
        int64_t result = 0;
        size_t i = start;

        const int sign = s[i] == '-' ? -1 : 1;
        if (s[i] == '-') i++;

        i += 2;

        while (i < pos && isxdigit(s[i])) {
            unsigned digit_val = TSC_JSON_XDIGIT[(unsigned char)s[i]];
            if (result > INT64_MAX / 16 || (result == INT64_MAX / 16 && digit_val > (INT64_MAX % 16))) {
                double dresult = (double)result;
                while (i < pos && isxdigit(s[i])) {
                    dresult = dresult * 16.0 + TSC_JSON_XDIGIT[(unsigned char)s[i]];
                    i++;
                }
                *out = tsc_number(dresult * sign);
                return 0;
            }
            result = result * 16 + digit_val;
            i++;
        }

        *out = tsc_int(result * sign);
    }
    else {
        int64_t result = 0;
        size_t i = start;

        const int sign = s[i] == '-' ? -1 : 1;
        if (s[i] == '-') i++;

        while (i < pos && isdigit(s[i])) {
            if (sign == -1) {
                if (result > INT64_MIN / -10 ||
                    (result == INT64_MIN / -10 && s[i] - '0' > -(INT64_MIN % 10))) {
                    double dresult = (double)result;
                    while (i < pos && isdigit(s[i])) {
                        dresult = dresult * 10.0 + (s[i] - '0');
                        i++;
                    }
                    *out = tsc_number(dresult * sign);
                    return 0;
                }
            }
            else {
                if (result > INT64_MAX / 10 || (result == INT64_MAX / 10 && s[i] - '0' > INT64_MAX % 10)) {
                    double dresult = (double)result;
                    while (i < pos && isdigit(s[i])) {
                        dresult = dresult * 10.0 + (s[i] - '0');
                        i++;
                    }
                    *out = tsc_number(dresult * sign);
                    return 0;
                }
            }
            result = result * 10 + (s[i] - '0');
            i++;
        }

        *out = tsc_int(result * sign);
    }
    return 1;
}

static tsc_json_error_t tsc_json_decode_any(
        const char *s,
        size_t *idx,
        tsc_value *out
) {
    TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), ;);
    const char c = s[(*idx)++];

    if (c == '{') {
        TSC_JSON_CALL(tsc_json_decode_object(s, idx, out), ;);
        return TSC_JSON_SUCCESS;
    }

    if (c == '[') {
        TSC_JSON_CALL(tsc_json_decode_array(s, idx, out), ;);
        return TSC_JSON_SUCCESS;
    }

    if (c == '"') {
        char* buf;
        TSC_JSON_CALL(tsc_json_decode_string(s, idx, &buf), ;);
        *out = tsc_string(buf);
        free(buf);
        return TSC_JSON_SUCCESS;
    }

    if (c ==           't' &&
        s[*idx]     == 'r' &&
        s[*idx + 1] == 'u' &&
        s[*idx + 2] == 'e'
    ) {
        *idx += 3;
        *out = tsc_boolean(1);
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'f' &&
        s[*idx]     == 'a' &&
        s[*idx + 1] == 'l' &&
        s[*idx + 2] == 's' &&
        s[*idx + 3] == 'e'
    ) {
        *idx += 4;
        *out = tsc_boolean(0);
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'n' &&
        s[*idx]     == 'u' &&
        s[*idx + 1] == 'l' &&
        s[*idx + 2] == 'l'
    ) {
        *idx += 3;
        *out = tsc_null();
        return TSC_JSON_SUCCESS;
    }

    if (c ==           'N' &&
        s[*idx]     == 'a' &&
        s[*idx + 1] == 'N'
    ) {
        *idx += 2;
        *out = tsc_number(NAN);
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
        *out = tsc_number(INFINITY);
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
        *out = tsc_number(-INFINITY);
        return TSC_JSON_SUCCESS;
    }

    (*idx)--;
    if (tsc_json_decode_number(s, idx, out)) return TSC_JSON_SUCCESS;
    return TSC_JSON_ERROR(EXPECTED_ANY, *idx);
}

tsc_value tsc_json_decode(
        const char* text,
        tsc_json_error_t* err
) {
    size_t idx = 0;
    tsc_value value;

    tsc_json_error_t result = tsc_json_decode_any(text, &idx, &value);
    if (err != NULL) *err = result;
    if (result.status != TSC_JSON_ERROR_SUCCESS) {
        return tsc_null();
    }

    result = tsc_json_skip_whitespace(text, &idx);
    if (err != NULL) *err = result;
    if (result.status != TSC_JSON_ERROR_SUCCESS)  {
        return tsc_null();
    }

    if (text[idx] != '\0') {
        if (err != NULL) *err = TSC_JSON_ERROR(TRAILING_CHARACTERS_AFTER_VALUE, idx);
        return tsc_null();
    }
    if (err != NULL) *err = TSC_JSON_SUCCESS;
    return value;
}
