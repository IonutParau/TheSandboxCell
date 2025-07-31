#include <stdlib.h>

#include "tscjson.h"
#include "tscjson_arrays.h"

#ifndef INFINITY
#define INFINITY (1.0f/0.0f)
#endif

#ifndef NAN
#define NAN (0.0f/0.0f)
#endif

#define TSC_JSON_CALL(thing, another_thing) do { \
    tsc_json_error_t result = thing; \
    if (result.status != TSC_JSON_ERROR_SUCCESS) { \
        another_thing; \
        return result; \
    } \
} while (0)

#define TSC_JSON_ERROR(err, idx) ((tsc_json_error_t) { (uint32_t)(TSC_JSON_PARSE_ERROR_##err), (uint32_t)(idx) })
#define TSC_JSON_SUCCESS ((tsc_json_error_t) { (uint32_t)(TSC_JSON_ERROR_SUCCESS), 0 })
#define TSC_JSON_PEEK_CHAR() (s[*idx])
#define TSC_JSON_TAKE_CHAR() (s[(*idx)++])
#define TSC_JSON_EXPECT_CHAR(expected_char, error_code, thing) do { \
    TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), thing); \
    if (TSC_JSON_TAKE_CHAR() != expected_char) { \
        thing; \
        return TSC_JSON_ERROR(error_code, *idx - 1); \
    } \
} while (0)

static void tsc_json_skip_line_comment(
        const char *s,
        size_t *idx
) {
    while (1) {
        const char c = TSC_JSON_TAKE_CHAR();
        if (c == '\0') (*idx)--;
        if (c == '\0' || c == '\n') break;
    }
}

static tsc_json_error_t tsc_json_skip_block_comment(
        const char *s,
        size_t *idx
) {
    const size_t start = *idx;
    while (1) {
        const char c = TSC_JSON_TAKE_CHAR();
        if (c == '\0') {
            return TSC_JSON_ERROR(UNTERMINATED_MULTILINE_COMMENT, start);
        }
        if (c == '*' && TSC_JSON_PEEK_CHAR() == '/') {
            TSC_JSON_TAKE_CHAR();
            return TSC_JSON_SUCCESS;
        }
    }
}

static tsc_json_error_t tsc_json_skip_whitespace(
        const char *s,
        size_t *idx
) {
    while (1) {
        char c = TSC_JSON_PEEK_CHAR();
        if (c == '\0') break;
        if (isspace(c)) {
            TSC_JSON_TAKE_CHAR();
            continue;
        }

        if (c == '/') {
            TSC_JSON_TAKE_CHAR();
            c = TSC_JSON_TAKE_CHAR();
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

static void tsc_json_utf8_encode(
        const unsigned int cp,
        tsc_buffer *buf
) {
    if (cp <= 0x7F) {
        tsc_saving_write(buf, cp);
    }
    else if (cp <= 0x7FF) {
        tsc_saving_write(buf, 0xC0 | (unsigned int)cp >> 6);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp & 0x3F);
    }
    else if (cp <= 0xFFFF) {
        tsc_saving_write(buf, 0xE0 | (unsigned int)cp >> 12);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp >> 6 & 0x3F);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp & 0x3F);
    }
    else {
        tsc_saving_write(buf, 0xF0 | (unsigned int)cp >> 18);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp >> 12 & 0x3F);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp >> 6 & 0x3F);
        tsc_saving_write(buf, 0x80 | (unsigned int)cp & 0x3F);
    }
}

static tsc_json_error_t tsc_json_decode_xXX(
        const char *s,
        size_t *idx,
        tsc_buffer *buf
) {
    if (!isxdigit(s[*idx]) ||
        !isxdigit(s[*idx + 1]) ) {
        return TSC_JSON_ERROR(INVALID_xXX_ESCAPE, *idx);
    }

    const unsigned int cp =
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 4 |
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]];

    *idx += 2;
    tsc_json_utf8_encode(cp, buf);
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_decode_uXXXX(
        const char *s,
        size_t *idx,
        tsc_buffer *buf
) {
    if (!isxdigit(s[*idx]) ||
        !isxdigit(s[*idx + 1]) ||
        !isxdigit(s[*idx + 2]) ||
        !isxdigit(s[*idx + 3])) {
        return TSC_JSON_ERROR(INVALID_uXXXX_ESCAPE, *idx);
    }

    const unsigned int cp =
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx]] << 12 |
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 1]] << 8 |
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 2]] << 4 |
        (unsigned int)TSC_JSON_XDIGIT[(unsigned char)s[*idx + 3]];

    *idx += 4;
    tsc_json_utf8_encode(cp, buf);
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_decode_UXXXXXXXX(
        const char *s,
        size_t *idx,
        tsc_buffer *buf
) {
    if (!isxdigit(s[*idx]) ||
        !isxdigit(s[*idx + 1]) ||
        !isxdigit(s[*idx + 2]) ||
        !isxdigit(s[*idx + 3]) ||
        !isxdigit(s[*idx + 4]) ||
        !isxdigit(s[*idx + 5]) ||
        !isxdigit(s[*idx + 6]) ||
        !isxdigit(s[*idx + 7])) {
        return TSC_JSON_ERROR(INVALID_UXXXXXXXX_ESCAPE, *idx);
    }

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
    tsc_json_utf8_encode(cp, buf);
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_decode_utf8(
        const char *s,
        size_t *idx,
        tsc_buffer *buf
) {
    const unsigned char b0 = (unsigned char)s[*idx];

    if (b0 == '\0') goto bad_utf8;

    if ((b0 & 0x80) == 0) {
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        return TSC_JSON_SUCCESS;
    }

    if ((b0 & 0xE0) == 0xC0) {
        if (s[*idx + 1] == '\0') goto bad_utf8;

        const unsigned char b1 = (unsigned char)s[*idx + 1];
        if ((b1 & 0xC0) != 0x80) goto bad_utf8;
        if (b0 < 0xC2) goto bad_utf8; // too long
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        return TSC_JSON_SUCCESS;
    }

    if ((b0 & 0xF0) == 0xE0) {
        if (s[*idx + 1] == '\0' || s[*idx + 2] == '\0') goto bad_utf8;

        const unsigned char b1 = (unsigned char)s[*idx + 1];
        const unsigned char b2 = (unsigned char)s[*idx + 2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) goto bad_utf8;
        if (b0 == 0xE0 && b1 < 0xA0) goto bad_utf8; // too long
        if (b0 == 0xED && b1 >= 0xA0) goto bad_utf8; // utf 16 surrogate range
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        return TSC_JSON_SUCCESS;
    }

    if ((b0 & 0xF8) == 0xF0) {
        if (s[*idx + 1] == '\0' || s[*idx + 2] == '\0' || s[*idx + 3] == '\0') goto bad_utf8;

        const unsigned char b1 = (unsigned char)s[*idx + 1];
        const unsigned char b2 = (unsigned char)s[*idx + 2];
        const unsigned char b3 = (unsigned char)s[*idx + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) goto bad_utf8;
        if (b0 == 0xF0 && b1 < 0x90) goto bad_utf8; // too long
        if (b0 > 0xF4 || (b0 == 0xF4 && b1 > 0x8F)) goto bad_utf8; // beyond unicode
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        tsc_saving_write(buf, TSC_JSON_TAKE_CHAR());
        return TSC_JSON_SUCCESS;
    }

bad_utf8:
    return TSC_JSON_ERROR(BAD_UTF8, *idx);
}

static tsc_json_error_t tsc_json_decode_string(
        const char *s,
        size_t *idx,
        char **out
) {
    tsc_buffer buf = tsc_saving_newBufferCapacity(NULL, 32);
    const size_t start = *idx - 1;

    while (1) {
        const char c = TSC_JSON_PEEK_CHAR();
        if ((unsigned char)c < ' ') {
            tsc_saving_deleteBuffer(buf);
            return TSC_JSON_ERROR(UNTERMINATED_STRING, start);
        }
        if (c == '"') {
            TSC_JSON_TAKE_CHAR();
            break;
        }
        if (c == '\\') {
            TSC_JSON_TAKE_CHAR();

            const char esc = TSC_JSON_TAKE_CHAR();
            if (esc == '\0') {
                tsc_saving_deleteBuffer(buf);
                return TSC_JSON_ERROR(UNTERMINATED_STRING, start);
            }

            if (TSC_JSON_BACKSLASH[(unsigned char)esc]) {
                tsc_saving_write(&buf, TSC_JSON_BACKSLASH[(unsigned char)esc]);
            }
            else if (esc == 'x') {
                TSC_JSON_CALL(tsc_json_decode_xXX(s, idx, &buf), tsc_saving_deleteBuffer(buf));
            }
            else if (esc == 'u') {
                TSC_JSON_CALL(tsc_json_decode_uXXXX(s, idx, &buf), tsc_saving_deleteBuffer(buf));
            }
            else if (esc == 'U') {
                TSC_JSON_CALL(tsc_json_decode_UXXXXXXXX(s, idx, &buf), tsc_saving_deleteBuffer(buf));
            }
            else {
                tsc_saving_deleteBuffer(buf);
                return TSC_JSON_ERROR(BAD_BACKSLASH, *idx);
            }
            continue;
        }
        TSC_JSON_CALL(tsc_json_decode_utf8(s, idx, &buf), tsc_saving_deleteBuffer(buf));
    }

    *out = buf.mem;
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

    if (TSC_JSON_PEEK_CHAR() == '}') {
        TSC_JSON_TAKE_CHAR();
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

        const char sep = TSC_JSON_TAKE_CHAR();
        if (sep == '}') return TSC_JSON_SUCCESS;
        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));
            if (TSC_JSON_PEEK_CHAR() == '}') {
                TSC_JSON_TAKE_CHAR();
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

    if (TSC_JSON_PEEK_CHAR() == ']') {
        TSC_JSON_TAKE_CHAR();
        return TSC_JSON_SUCCESS;
    }

    while (1) {
        tsc_value value;
        TSC_JSON_CALL(tsc_json_decode_any(s, idx, &value), tsc_destroy(*out));
        tsc_append(*out, value);
        tsc_destroy(value);

        TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));


        const char sep = TSC_JSON_TAKE_CHAR();
        if (sep == ']') return TSC_JSON_SUCCESS;

        if (sep == ',') {
            TSC_JSON_CALL(tsc_json_skip_whitespace(s, idx), tsc_destroy(*out));
            if (TSC_JSON_PEEK_CHAR() == ']') {
                TSC_JSON_TAKE_CHAR();
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
        double result = 0;
        size_t i = start;

        const double sign = s[i] == '-' ? -1 : 1;
        if (s[i] == '-') i++;

        // i[s] because i am going mentally insane right now
        if (i[s] == '.') {
            i++;
            const int ib = i;
            while (isdigit(s[i])) {
                result = result * 10.0 + (s[i] - '0');
                i++;
            }
            result /= TSC_JSON_POWER_OF_10[ib - i];
        }
        else if (isdigit(s[i])) {
            while (isdigit(s[i])) {
                result = result * 10.0 + (s[i] - '0');
                i++;
            }

            if (s[i] == '.') {
                i++;
                double fraction = 0.0;
                const int ib = i;
                while (isdigit(s[i])) {
                    fraction = fraction * 10.0 + (s[i] - '0');
                    i++;
                }
                result += fraction / TSC_JSON_POWER_OF_10[i - ib];
            }
        }
        if (s[i] == 'e' || s[i] == 'E') {
            i++;
            int exp_sign = 1;
            if (s[i] == '+') {
                i++;
            }
            else if (s[i] == '-') {
                exp_sign = -1;
                i++;
            }

            int exponent = 0;
            while (isdigit(s[i])) {
                exponent = exponent * 10 + (s[i] - '0');
                i++;
            }

            const int exp_val = exp_sign * exponent;
            if (exp_val != 0) {
                int n = exp_val < 0 ? -exp_val : exp_val;
                double factor = 1.0;
                double base_power = 10.0;
                while (n) {
                    if (n & 1) {
                        factor *= base_power;
                    }
                    base_power *= base_power;
                    n >>= 1;
                }
                if (exp_val < 0) {
                    result /= factor;
                }
                else {
                    result *= factor;
                }
            }
        }

        *out = tsc_number(result * sign);
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
    const char c = TSC_JSON_TAKE_CHAR();

    if (c == '{') {
        TSC_JSON_CALL(tsc_json_decode_object(s, idx, out), ;);
        return TSC_JSON_SUCCESS;
    }

    if (c == '[') {
        TSC_JSON_CALL(tsc_json_decode_array(s, idx, out), ;);
        return TSC_JSON_SUCCESS;
    }

    if (c == '"') {
        char* str;
        TSC_JSON_CALL(tsc_json_decode_string(s, idx, &str), ;);
        *out = tsc_string(str);
        free(str);
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
