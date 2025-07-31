#include <stdio.h>

#include "tscjson.h"
#include "tscjson_arrays.h"

#define TSC_JSON_CALL(thing) do { \
    tsc_json_error_t result = thing; \
    if (result.status != TSC_JSON_ERROR_SUCCESS) { \
        return result; \
    } \
} while (0)

#define TSC_JSON_ERROR(err) ((tsc_json_error_t) { (uint32_t)(TSC_JSON_ENCODE_ERROR_##err), 0 })
#define TSC_JSON_SUCCESS ((tsc_json_error_t) { (uint32_t)(TSC_JSON_ERROR_SUCCESS), 0 })
#define TSC_SAVING_WRITESPACE(buf, amount) do { \
    if ((amount)) { \
        tsc_saving_writeFormat(buf, "%*c", (amount), ' '); \
    } \
} while (0)

static void memcpy8(void *dst, const void *src) {
    *(uint64_t *) dst = *(const uint64_t *) src;
}

static int tsc_isnan(const double d) {
    uint64_t bits;
    memcpy8(&bits, &d);
    return bits >> 52 == 0x7FF && (bits & ((1ULL << 52) - 1)) != 0;
}

static int tsc_isinf(const double d) {
    uint64_t bits;
    memcpy8(&bits, &d);
    return bits == 0x7FF0000000000000;
}

// these two functions are stupid shitcode
// if you can please improve this
static void tsc_saving_writeNumber(tsc_buffer* buf, const double d) {
    char buffer[64];
    char *p = buffer;

    const uint64_t int_part = (uint64_t)d;
    double frac_part = d - int_part;

    if (int_part == 0) {
        *p++ = '0';
    }
    else {
        char *start = p;
        uint64_t temp = int_part;

        do {
            *p++ = '0' + temp % 10;
            temp /= 10;
        } while (temp);

        for (char *end = p - 1; start < end; ) {
            const char tmp = *start;
            *start++ = *end;
            *end-- = tmp;
        }
    }

    *p++ = '.';

    const char *frac_start = p;

    for (int i = 0; i < 15; ++i) {
        if (frac_part <= 1e-15) break;
        frac_part *= 10;
        int digit = (int)frac_part;
        *p++ = '0' + digit;
        frac_part -= digit;
    }

    while (p > frac_start && p[-1] == '0') p--;
    if (p == frac_start) *p++ = '0';

    tsc_saving_writeBytes(buf, buffer, p - buffer);
}

void tsc_saving_writeNumberScientific(
        tsc_buffer *buf,
        const double num
) {
    int exp = 0;

    if (num >= 1.0) {
        while (exp <= 308 && TSC_JSON_POWER_OF_10[exp + 1] <= num) exp++;
    }
    else {
        while (exp >= -324 && TSC_JSON_POWER_OF_10[exp] > num) exp--;
    }

    char digits[16];
    int digit_count = 0;
    double rem = num;

    for (int pos = exp; pos >= exp - 15 && rem > 0.0 && digit_count < 15; pos--) {
        const double div = TSC_JSON_POWER_OF_10[pos];
        const int digit = (int)(rem / div);
        if (digit > 0 || digit_count > 0) {
            digits[digit_count++] = '0' + digit;
            rem -= digit * div;
        }

        if (rem < div * 1e-15) break;
    }

    while (digit_count > 1 && digits[digit_count - 1] == '0') digit_count--;

    tsc_saving_write(buf, digits[0]);

    if (digit_count > 1) {
        tsc_saving_write(buf, '.');
        tsc_saving_writeBytes(buf, digits + 1, digit_count - 1);
    }

    if (exp != 0) {
        tsc_saving_write(buf, 'e');
        tsc_saving_writeFormat(buf, "%d", exp);
    }
}

static void tsc_json_encode_number(
        tsc_buffer *buf,
        double d
) {
    if (d < 0) {
        tsc_saving_write(buf, '-');
        d *= -1;
    }
    if (tsc_isnan(d)) {
        tsc_saving_writeStr(buf, "NaN");
    }
    else if (tsc_isinf(d)) {
        tsc_saving_writeStr(buf, "Infinity");
    }
    else if (d == 0.0 || (0.0001 < d && d < 1e15)) {
        // for "normal" numbers print with up to 15 decimal places
        tsc_saving_writeNumber(buf, d);
    }
    else {
        // use scientific notation for everything else
        tsc_saving_writeNumberScientific(buf, d);
    }
}

static int tsc_json_encode_char(
        tsc_buffer *buf,
        const char *s,
        const bool ensure_ascii
) {
    const unsigned char b0 = (unsigned char) s[0];

    if ((b0 & 0x80) == 0) {
        if (TSC_JSON_ESCAPES[b0]) {
            tsc_saving_writeStr(buf, TSC_JSON_ESCAPES[b0]);
        }
        else {
            tsc_saving_write(buf, b0);
        }
        return 1;
    }

    if ((b0 & 0xE0) == 0xC0) {
        if (!s[1]) return 0;

        const unsigned char b1 = (unsigned char)s[1];
        if ((b1 & 0xC0) != 0x80) return 0;
        if (b0 < 0xC2) return 0; // too long
        if (ensure_ascii) {
            const unsigned int cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            tsc_saving_writeFormat(buf, "\\u%04X", cp);
        }
        else {
            tsc_saving_write(buf, b0);
            tsc_saving_write(buf, b1);
        }
        return 2;
    }

    if ((b0 & 0xF0) == 0xE0) {
        if (!s[1] || !s[2]) return 0;

        const unsigned char b1 = (unsigned char)s[1];
        const unsigned char b2 = (unsigned char)s[2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return 0;
        if (b0 == 0xE0 && b1 < 0xA0) return 0; // too long
        if (b0 == 0xED && b1 >= 0xA0) return 0; // utf 16 surrogate range
        if (ensure_ascii) {
            const unsigned int cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            if (cp <= 0xFFFF) {
                tsc_saving_writeFormat(buf, "\\u%04X", cp);
            }
            else {
                tsc_saving_writeFormat(buf, "\\U%08X", cp);
            }
        }
        else {
            tsc_saving_write(buf, b0);
            tsc_saving_write(buf, b1);
            tsc_saving_write(buf, b2);
        }
        return 3;
    }

    if ((b0 & 0xF8) == 0xF0) {
        if (!s[1] || !s[2] || !s[3]) return 0;

        const unsigned char b1 = (unsigned char)s[1];
        const unsigned char b2 = (unsigned char)s[2];
        const unsigned char b3 = (unsigned char)s[3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return 0;
        if (b0 == 0xF0 && b1 < 0x90) return 0; // too long
        if (b0 > 0xF4 || (b0 == 0xF4 && b1 > 0x8F)) return 0; // beyond unicode
        if (ensure_ascii) {
            const unsigned int cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

            if (cp <= 0xFFFF) {
                tsc_saving_writeFormat(buf, "\\u%04X", cp);
            }
            else {
                tsc_saving_writeFormat(buf, "\\U%08X", cp);
            }
        }
        else {
            tsc_saving_write(buf, b0);
            tsc_saving_write(buf, b1);
            tsc_saving_write(buf, b2);
            tsc_saving_write(buf, b3);
        }
        return 4;
    }

    return 0;
}

static tsc_json_error_t tsc_json_encode_string(
        tsc_buffer *buf,
        const char *s,
        const bool ensure_ascii
) {
    tsc_saving_write(buf, '"');
    while (*s) {
        const int consumed = tsc_json_encode_char(buf, s, ensure_ascii);
        if (consumed == 0) {
            return TSC_JSON_ERROR(INVALID_STRING);
        }
        s += consumed;
    }
    tsc_saving_write(buf, '"');
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_encode_any(
        tsc_buffer *buf,
        const tsc_value v,
        const int current_indent,
        const int indent,
        const bool ensure_ascii
);

static tsc_json_error_t tsc_json_encode_object(
        tsc_buffer *buf,
        const tsc_object_t *o,
        const int current_indent,
        const int indent,
        const bool ensure_ascii
) {
    tsc_saving_write(buf, '{');
    if (o->len == 0) {
        tsc_saving_write(buf, '}');
        return TSC_JSON_SUCCESS;
    }

    if (indent) {
        tsc_saving_write(buf, '\n');
    }

    bool first = true;
    for (int i = 0; i < o->len; i++) {
        if (!first) {
            tsc_saving_write(buf, ',');
            if (indent) {
                tsc_saving_write(buf, '\n');
            }
            else {
                tsc_saving_write(buf, ' ');
            }
        }
        first = false;

        TSC_SAVING_WRITESPACE(buf, (current_indent + 1) * indent);

        TSC_JSON_CALL(tsc_json_encode_string(buf, o->keys[i], ensure_ascii));
        tsc_saving_writeStr(buf, ": ");
        TSC_JSON_CALL(tsc_json_encode_any(buf, o->values[i], current_indent + 1, indent, ensure_ascii));
    }

    if (indent > 0) {
        tsc_saving_write(buf, '\n');
        TSC_SAVING_WRITESPACE(buf, current_indent * indent);
    }
    tsc_saving_write(buf, '}');
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_encode_array(
        tsc_buffer *buf,
        const tsc_array_t *a,
        const int current_indent,
        const int indent,
        const bool ensure_ascii
) {
    tsc_saving_write(buf, '[');
    if (a->valuec == 0) {
        tsc_saving_write(buf, ']');
        return TSC_JSON_SUCCESS;
    }

    if (indent) {
        tsc_saving_write(buf, '\n');
    }

    bool first = true;
    for (int i = 0; i < a->valuec; i++) {
        if (!first) {
            tsc_saving_write(buf, ',');
            if (indent) {
                tsc_saving_write(buf, '\n');
            }
            else {
                tsc_saving_write(buf, ' ');
            }
        }
        first = false;

        TSC_SAVING_WRITESPACE(buf, (current_indent + 1) * indent);

        TSC_JSON_CALL(tsc_json_encode_any(buf, a->values[i], current_indent + 1, indent, ensure_ascii));
    }

    if (indent) {
        tsc_saving_write(buf, '\n');
        TSC_SAVING_WRITESPACE(buf, current_indent * indent);
    }
    tsc_saving_write(buf, ']');
    return TSC_JSON_SUCCESS;
}

static tsc_json_error_t tsc_json_encode_any(
        tsc_buffer *buf,
        const tsc_value v,
        const int current_indent,
        const int indent,
        const bool ensure_ascii
) {
    if (tsc_isNull(v)) {
        tsc_saving_writeStr(buf, "null");
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isBoolean(v)) {
        tsc_saving_writeStr(buf, tsc_toBoolean(v) ? "true" : "false");
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isInt(v)) {
        tsc_saving_writeFormat(buf, "%ld", tsc_toInt(v));
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isNumber(v)) {
        tsc_json_encode_number(buf, tsc_toNumber(v));
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isString(v)) {
        TSC_JSON_CALL(tsc_json_encode_string(buf, tsc_toString(v), ensure_ascii));
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isArray(v)) {
        TSC_JSON_CALL(tsc_json_encode_array(buf, v.array, current_indent, indent, ensure_ascii));
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isObject(v)) {
        TSC_JSON_CALL(tsc_json_encode_object(buf, v.object, current_indent, indent, ensure_ascii));
        return TSC_JSON_SUCCESS;
    }

    if (tsc_isCell(v)) {
        return TSC_JSON_ERROR(CANT_ENCODE_CELL);
    }
    // i don't think i should return success here,
    // but you are doing something really wrong for sure
    // if you managed to get here
    return TSC_JSON_SUCCESS;
}

tsc_buffer tsc_json_encode(
    const tsc_value value,
    tsc_json_error_t *err,
    const int indent,
    const bool ensure_ascii
) {
    tsc_buffer buf = tsc_saving_newBuffer(NULL);
    const tsc_json_error_t result = tsc_json_encode_any(&buf, value, 0, indent, ensure_ascii);
    if (err != NULL) *err = result;
    if (result.status != TSC_JSON_ERROR_SUCCESS) {
        tsc_saving_deleteBuffer(buf);
    }
    return buf;
}
