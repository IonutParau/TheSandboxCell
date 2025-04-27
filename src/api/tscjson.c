#include "tscjson.h"
#include "value.h"
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>

static void tsc_json_encodeStringInto(tsc_buffer *buffer, const char *str) {
    tsc_saving_write(buffer, '"');
    for(size_t i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if(c == '"') {
            tsc_saving_writeStr(buffer, "\\\"");
        } else if(c == '\n') {
            tsc_saving_writeStr(buffer, "\\n");
        } else if(c == '\t') {
            tsc_saving_writeStr(buffer, "\\t");
        } else if(c == '\r') {
            tsc_saving_writeStr(buffer, "\\r");
        } else if(c == '\f') {
            tsc_saving_writeStr(buffer, "\\f");
        } else if(c == '\b') {
            tsc_saving_writeStr(buffer, "\\b");
        } else if(c == '\v') {
            tsc_saving_writeStr(buffer, "\\v");
        } else if(c == '\\') {
            tsc_saving_writeStr(buffer, "\\\\");
        } else {
            tsc_saving_write(buffer, c);
        }
    }
    tsc_saving_write(buffer, '"');
}

static int tsc_json_encodeValueInto(tsc_value value, tsc_buffer *buffer, tsc_buffer *err) {
    if(tsc_isNull(value)) {
        tsc_saving_writeStr(buffer, "null");
        return 0;
    } else if(tsc_isInt(value)) {
        int64_t num = tsc_toInt(value);
        tsc_saving_writeFormat(buffer, "%ld", num);
    } else if(tsc_isNumber(value)) {
        double num = tsc_toNumber(value);
        tsc_saving_writeFormat(buffer, "%lf", num);
    } else if(tsc_isString(value)) {
        tsc_json_encodeStringInto(buffer, tsc_toString(value));
    } else if(tsc_isCell(value)) {
        if(err != NULL) tsc_saving_writeFormat(err, "Unencodable value: Cell");
        return 1;
    } else if(tsc_isBoolean(value)) {
        if(tsc_toBoolean(value)) {
            tsc_saving_writeStr(buffer, "true");
        } else {
            tsc_saving_writeStr(buffer, "false");
        }
    } else if(tsc_isArray(value)) {
        tsc_saving_write(buffer, '[');
        size_t len = tsc_getLength(value);
        for(size_t i = 0; i < len; i++) {
            int verr = tsc_json_encodeValueInto(tsc_index(value, i), buffer, err);
            if(verr != 0) return verr;
            if(i < len-1) {
                tsc_saving_write(buffer, ',');
            }
        }
        tsc_saving_write(buffer, ']');
    } else if(tsc_isObject(value)) {
        tsc_saving_write(buffer, '{');
        size_t len = tsc_getLength(value);
        for(size_t i = 0; i < len; i++) {
            tsc_json_encodeStringInto(buffer, tsc_keyAt(value, i));
            tsc_saving_write(buffer, ':');
            int verr = tsc_json_encodeValueInto(tsc_index(value, i), buffer, err);
            if(verr != 0) return verr;
            if(i < len-1) {
                tsc_saving_write(buffer, ',');
            }
        }
        tsc_saving_write(buffer, '}');
    }
    return 0;
}

tsc_buffer tsc_json_encode(tsc_value value, tsc_buffer *err) {
    tsc_buffer buf = tsc_saving_newBuffer("");

    tsc_json_encodeValueInto(value, &buf, err);

    return buf;
}

static char tsc_json_peek(const char **text) {
    return **text;
}

static char tsc_json_next(const char **text) {
    char c = tsc_json_peek(text);
    if(c != '\0') {
        *text = *text + 1;
    }
    return c;
}

static int tsc_json_skipWhitespace(const char **text, tsc_buffer *err) {
    while(true) {
        char c = tsc_json_peek(text);
        if(c == '\0') break;
        if(isspace(c)) {
            tsc_json_next(text);
            continue;
        }
        if(c == '/') {
            tsc_json_next(text);
            c = tsc_json_next(text);
            if(c == '/') {
                while(true) {
                    char ch = tsc_json_next(text);
                    if(ch == '\0' || ch == '\n') break;
                }
                continue;
            } else if(c == '*') {
                while(true) {
                    char ch = tsc_json_next(text);
                    if(ch == '\0') break;
                    if(ch == '*') {
                        char nch = tsc_json_next(text);
                        if(nch == '\0') break;
                        if(nch == '/') break;
                    }
                }
                continue;
            } else {
                if(err) tsc_saving_writeFormat(err, "Bad character: %c (0x%x)", c, (int)c);
                return 1;
            }
        }
        break;
    }
    return 0;
}

static int tsc_json_decodeString(const char **text, tsc_buffer *out, tsc_buffer *err) {
    char e = tsc_json_next(text); // skip "

    while(true) {
        char c = tsc_json_next(text);
        if(c == e) break;
        if(c == '\0') {
            if(err != NULL) tsc_saving_writeStr(err, "String literal ends too early");
            return 1;
        }
        if(c == '\\') {
            char n = tsc_json_next(text);
            if(n == 'n') {
                tsc_saving_write(out, '\n');
            } else if(n == 'v') {
                tsc_saving_write(out, '\v');
            } else if(n == '\\') {
                tsc_saving_write(out, '\\');
            } else if(n == 't') {
                tsc_saving_write(out, '\t');
            } else if(n == 'r') {
                tsc_saving_write(out, '\r');
            } else if(n == 'f') {
                tsc_saving_write(out, '\f');
            } else if(n == 'b') {
                tsc_saving_write(out, '\b');
            } else {
                tsc_saving_write(out, n);
            }
        } else {
            tsc_saving_write(out, c);
        }
    }

    return 0;
}

static int tsc_json_decodeSignedNumber(const char **text, double *out, tsc_buffer *err);

static int tsc_json_hexdigit(char c) {
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return c - '0';
}

static int tsc_json_decodeNumber(const char **text, double *out, tsc_buffer *err) {
    char c = tsc_json_next(text);
    bool shouldHaveDot = true;
    *out = 0;
    if(c == '0') {
        char n = tsc_json_peek(text);
        if(n == 'x') {
            tsc_json_next(text);
            shouldHaveDot = false;
            while(isxdigit(tsc_json_peek(text))) {
                char h = tsc_json_next(text);
                *out *= 16;
                *out += tsc_json_hexdigit(h);
            }
        } else if(n == 'b') {
            tsc_json_next(text);
            shouldHaveDot = false;
            while((tsc_json_peek(text) == '0') || (tsc_json_peek(text) == '1')) {
                char h = tsc_json_next(text);
                *out *= 2;
                *out += h == '1';
            }
        } else if(isdigit(n)) {
            // This works because ASCII
            while(isdigit(tsc_json_peek(text))) {
                *out *= 10;
                *out += tsc_json_next(text) - '0';
            }
        }
    } else if(isdigit(c)) {
        *out = c - '0';
        while(isdigit(tsc_json_peek(text))) {
            *out *= 10;
            *out += tsc_json_next(text) - '0';
        }
    } else {
        if(err != NULL) tsc_saving_writeStr(err, "Bad digit");
        return 1;
    }
    if(tsc_json_skipWhitespace(text, err) != 0) return 1;
    if(shouldHaveDot) {
        if(tsc_json_peek(text) == '.') {
            tsc_json_next(text);
            double d = 0.1;
            while(isdigit(tsc_json_peek(text))) {
                *out += (tsc_json_next(text) - '0') * d;
                d = d / 10;
            }
        }
    }
    if(tsc_json_peek(text) == 'e') {
        tsc_json_next(text);
        double power;
        if(tsc_json_decodeSignedNumber(text, &power, err) != 0) return 1;
        *out = pow(*out, power);
    }
    return 0;
}

static int tsc_json_decodeSignedNumber(const char **text, double *out, tsc_buffer *err) {
    if(tsc_json_peek(text) == '-') {
        tsc_json_next(text);
        if(tsc_json_skipWhitespace(text, err) != 0) return 1;
        double n;
        if(tsc_json_decodeNumber(text, &n, err) != 0) return 1;
        *out = -n;
        return 0;
    }
    return tsc_json_decodeNumber(text, out, err);
}

static int tsc_json_decodeValue(tsc_value *value, const char **text, tsc_buffer *err) {
    if(tsc_json_skipWhitespace(text, err) != 0) return 1;
    char c = tsc_json_peek(text);
    if(c == '\'' || c == '"') {
        tsc_buffer out = tsc_saving_newBuffer("");
        int e = tsc_json_decodeString(text, &out, err);
        if(e != 0) {
            tsc_saving_deleteBuffer(out);
            return e;
        }
        *value = tsc_lstring(out.mem, out.len);
        tsc_saving_deleteBuffer(out);
        return 0;
    }
    if(c == '-' || isdigit(c)) {
        double d;
        if(tsc_json_decodeSignedNumber(text, &d, err) != 0) return 1;
        *value = tsc_number(d);
        return 0;
    }
    if(c == 't') {
        if(tsc_json_next(text) != 't') return 1;
        if(tsc_json_next(text) != 'r') return 1;
        if(tsc_json_next(text) != 'u') return 1;
        if(tsc_json_next(text) != 'e') return 1;
        *value = tsc_boolean(true);
        return 0;
    }
    if(c == 'f') {
        if(tsc_json_next(text) != 'f') return 1;
        if(tsc_json_next(text) != 'a') return 1;
        if(tsc_json_next(text) != 'l') return 1;
        if(tsc_json_next(text) != 's') return 1;
        if(tsc_json_next(text) != 'e') return 1;
        *value = tsc_boolean(false);
        return 0;
    }
    if(c == 'n') {
        if(tsc_json_next(text) != 'n') return 1;
        if(tsc_json_next(text) != 'u') return 1;
        if(tsc_json_next(text) != 'l') return 1;
        if(tsc_json_next(text) != 'l') return 1;
        *value = tsc_null();
        return 0;
    }
    if(c == '[') {
        tsc_json_next(text);
        tsc_value arr = tsc_array(0);
        while(true) {
            while(true) {
                if(tsc_json_skipWhitespace(text, err) != 0) {
                    tsc_destroy(arr);
                    return 1;
                }
                if(tsc_json_peek(text) == ',') {
                    tsc_json_next(text);
                    continue;
                }
                break;
            }
            if(tsc_json_skipWhitespace(text, err) != 0) {
                tsc_destroy(arr);
                return 1;
            }
            if(tsc_json_peek(text) == ']') {
                tsc_json_next(text);
                break;
            }
            tsc_value v = tsc_null();
            if(tsc_json_decodeValue(&v, text, err) != 0) {
                tsc_destroy(arr);
                tsc_destroy(v);
                return 1;
            }
            tsc_append(arr, v);
        }
        *value = arr;
        return 0;
    }
    if(c == '{') {
        tsc_json_next(text);
        tsc_value obj = tsc_object();
        while(true) {
            while(true) {
                if(tsc_json_skipWhitespace(text, err) != 0) {
                    tsc_destroy(obj);
                    return 1;
                }
                if(tsc_json_peek(text) == ',') {
                    tsc_json_next(text);
                    continue;
                }
                break;
            }
            if(tsc_json_skipWhitespace(text, err) != 0) {
                tsc_destroy(obj);
                return 1;
            }
            if(tsc_json_peek(text) == '}') {
                tsc_json_next(text);
                break;
            }
            tsc_buffer field = tsc_saving_newBuffer("");
            if(tsc_json_decodeString(text, &field, err) != 0) {
                tsc_destroy(obj);
                return 1;
            }
            if(tsc_json_skipWhitespace(text, err) != 0) {
                tsc_destroy(obj);
                return 1;
            }
            if(tsc_json_peek(text) == ':') {
                tsc_json_next(text);
            }
            if(tsc_json_skipWhitespace(text, err) != 0) {
                tsc_destroy(obj);
                return 1;
            }
            tsc_value v = tsc_null();
            if(tsc_json_decodeValue(&v, text, err) != 0) {
                tsc_destroy(obj);
                tsc_destroy(v);
                return 1;
            }
            tsc_setKey(obj, field.mem, v);
            tsc_saving_deleteBuffer(field);
        }
        *value = obj;
        return 0;
    }
    printf("Bad char: %c (0x%x)\n", c, (int)c);
    if(c == '\0') {
        if(err != NULL) tsc_saving_writeFormat(err, "Missing value");
        return 1;
    }
    if(err != NULL) tsc_saving_writeFormat(err, "Bad char: %c (0x%x)", c, (int)c);
    return 1;
}

tsc_value tsc_json_decode(const char *text, tsc_buffer *err) {
    tsc_value value = tsc_null();
    if(tsc_json_decodeValue(&value, &text, err) != 0) {
        return tsc_null();
    }
    return value;
}
