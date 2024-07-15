#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "value.h"

tsc_value tsc_null() {
    tsc_value v;
    v.tag = TSC_VALUE_NULL;
    return v;
}

tsc_value tsc_int(int64_t integer) {
    tsc_value v;
    v.tag = TSC_VALUE_INT;
    v.integer = integer;
    return v;
}

tsc_value tsc_number(double number) {
    tsc_value v;
    v.tag = TSC_VALUE_NUMBER;
    v.number = number;
    return v;
}

tsc_value tsc_boolean(bool boolean) {
    tsc_value v;
    v.tag = TSC_VALUE_BOOL;
    v.boolean = boolean;
    return v;
}

tsc_value tsc_string(const char *str) {
    return tsc_lstring(str, strlen(str));
}

tsc_value tsc_lstring(const char *str, size_t len) {
    tsc_value v;
    v.tag = TSC_VALUE_STRING;
    tsc_string_t *string = malloc(sizeof(tsc_string_t));
    string->refc = 1;
    // Null terminator is guaranteed.
    string->memory = malloc(sizeof(char) * (len) + 1);
    memcpy(string->memory, str, sizeof(char) * len);
    string->memory[len] = '\0';
    string->len = len;
    v.string = string;
    return v;
}

tsc_value tsc_cstring(const char *str) {
    tsc_value v;
    v.tag = TSC_VALUE_CSTRING;
    v.cstring = str;
    return v;
}

tsc_value tsc_array(size_t len) {
    tsc_value v;
    v.tag = TSC_VALUE_ARRAY;
    tsc_array_t *array = malloc(sizeof(tsc_array_t));
    array->refc = 1;
    array->valuec = len;
    array->values = malloc(sizeof(tsc_value) * len);
    for(size_t i = 0; i < len; i++) {
        array->values[i] = tsc_null();
    }
    v.array = array;
    return v;
}

tsc_value tsc_object() {
    tsc_value v;
    v.tag = TSC_VALUE_OBJECT;
    tsc_object_t *object = malloc(sizeof(tsc_object_t));
    object->refc = 1;
    object->len = 0;
    object->keys = NULL;
    object->values = NULL;
    v.object = object;
    return v;
}

void tsc_retain(tsc_value value) {
    // TODO: retain
}

void tsc_destroy(tsc_value value) {
    // TODO: destroy
}

tsc_value tsc_index(tsc_value list, size_t index) {
    if(list.tag == TSC_VALUE_ARRAY) {
        if(index >= list.array->valuec) return tsc_null();
        return list.array->values[index];
    }
    if(list.tag == TSC_VALUE_OBJECT) {
        if(index >= list.object->len) return tsc_null();
        return list.object->values[index];
    }

    return tsc_null();
}

void tsc_setIndex(tsc_value list, size_t index, tsc_value value) {
    if(list.tag == TSC_VALUE_ARRAY) {
        if(index >= list.array->valuec) return;
        tsc_retain(value);
        tsc_destroy(list.array->values[index]);
        list.array->values[index] = value;
        return;
    }
    if(list.tag == TSC_VALUE_OBJECT) {
        if(index >= list.object->len) return;
        tsc_retain(value);
        tsc_destroy(list.object->values[index]);
        list.object->values[index] = value;
        return;
    }
}

tsc_value tsc_getKey(tsc_value object, const char *key) {
    if(object.tag != TSC_VALUE_OBJECT) return tsc_null();

    for(size_t i = 0; i < object.object->len; i++) {
        if(strcmp(object.object->keys[i], key) == 0) {
            return object.object->values[i];
        }
    }

    return tsc_null();
}

void tsc_setKey(tsc_value object, const char *key, tsc_value value) {
    if(object.tag != TSC_VALUE_OBJECT) return;

    for(size_t i = 0; i < object.object->len; i++) {
        if(strcmp(object.object->keys[i], key) == 0) {
            tsc_retain(value);
            tsc_destroy(object.object->values[i]);
            object.object->values[i] = value;
            return;
        }
    }

    size_t idx = object.object->len++;
    object.object->values = realloc(object.object->values, sizeof(tsc_value) * object.object->len);
    tsc_retain(value);
    object.object->values[idx] = value;
    object.object->keys = realloc(object.object->keys, sizeof(char *) * object.object->len);
    object.object->keys[idx] = tsc_strdup(key);
}

bool tsc_isInt(tsc_value value) {
    return value.tag == TSC_VALUE_INT;
}

bool tsc_isNumber(tsc_value value) {
    return value.tag == TSC_VALUE_NUMBER;
}

bool tsc_isNumerical(tsc_value value) {
    return tsc_isInt(value) || tsc_isNumber(value);
}

bool tsc_isBoolean(tsc_value value) {
    return value.tag == TSC_VALUE_BOOL;
}

bool tsc_isString(tsc_value value) {
    return value.tag == TSC_VALUE_STRING || value.tag == TSC_VALUE_CSTRING;
}

bool tsc_isArray(tsc_value value) {
    return value.tag == TSC_VALUE_ARRAY;
}

bool tsc_isObject(tsc_value value) {
    return value.tag == TSC_VALUE_OBJECT;
}

int64_t tsc_toInt(tsc_value value) {
    if(value.tag == TSC_VALUE_INT) return value.integer;
    if(value.tag == TSC_VALUE_NUMBER) return value.number;
    return 0;
}

double tsc_toNumber(tsc_value value) {
    if(value.tag == TSC_VALUE_INT) return value.integer;
    if(value.tag == TSC_VALUE_NUMBER) return value.number;
    return 0;
}

bool tsc_toBoolean(tsc_value value) {
    if(value.tag == TSC_VALUE_NULL) return false;
    if(value.tag == TSC_VALUE_INT) return value.integer != 0;
    if(value.tag == TSC_VALUE_NUMBER) return value.number != 0;
    if(value.tag == TSC_VALUE_BOOL) return value.boolean;
    if(value.tag == TSC_VALUE_STRING) return value.string->len != 0;
    if(value.tag == TSC_VALUE_CSTRING) return strlen(value.cstring) != 0;
    if(value.tag == TSC_VALUE_ARRAY) return value.array->valuec != 0;
    if(value.tag == TSC_VALUE_OBJECT) return value.object->len != 0;
    return false;
}

const char *tsc_toString(tsc_value value) {
    return tsc_toLString(value, NULL);
}

const char *tsc_toLString(tsc_value value, size_t *len) {
    if(value.tag == TSC_VALUE_STRING) {
        if(len != NULL) *len = value.string->len;
        return value.string->memory;
    }

    if(value.tag == TSC_VALUE_CSTRING) {
        if(len != NULL) *len = strlen(value.cstring);
        return value.cstring;
    }

    if(len != NULL) *len = 0;
    return "";
}

size_t tsc_getLength(tsc_value value) {
    if(value.tag == TSC_VALUE_STRING) return value.string->len;
    if(value.tag == TSC_VALUE_CSTRING) return strlen(value.cstring);
    if(value.tag == TSC_VALUE_ARRAY) return value.array->valuec;
    if(value.tag == TSC_VALUE_OBJECT) return value.object->len;
    return 0;
}

const char *tsc_keyAt(tsc_value object, size_t index) {
    if(object.tag != TSC_VALUE_OBJECT) return NULL;
    if(index >= object.object->len) return NULL;
    return object.object->keys[index];
}
