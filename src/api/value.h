#ifndef TSC_VALUE_H
#define TSC_VALUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../cells/grid.h"

// Signal values

#define TSC_VALUE_NULL 0
#define TSC_VALUE_INT 1
#define TSC_VALUE_NUMBER 2
#define TSC_VALUE_BOOL 3
#define TSC_VALUE_STRING 4
#define TSC_VALUE_CSTRING 5
#define TSC_VALUE_ARRAY 6
#define TSC_VALUE_OBJECT 7
#define TSC_VALUE_CELLPTR 8
#define TSC_VALUE_OWNEDCELL 9

typedef struct tsc_value tsc_value;

typedef struct tsc_string_t {
    size_t refc;
    char *memory;
    size_t len;
} tsc_string_t;

typedef struct tsc_array_t {
    size_t refc;
    tsc_value *values;
    size_t valuec;
} tsc_array_t;

typedef struct tsc_object_t {
    size_t refc;
    tsc_value *values;
    char **keys;
    size_t len;
} tsc_object_t;

typedef struct tsc_ownedcell_t {
    size_t refc;
    tsc_cell *cell;
} tsc_ownedcell_t;

typedef struct tsc_value {
    size_t tag;
    union {
        int64_t integer;
        double number;
        bool boolean;
        tsc_string_t *string;
        const char *cstring;
        tsc_array_t *array;
        tsc_object_t *object;
        tsc_cell *cellptr;
        tsc_ownedcell_t *ownedcell;
    };
} tsc_value;

tsc_value tsc_null();
tsc_value tsc_int(int64_t integer);
tsc_value tsc_number(double number);
tsc_value tsc_boolean(bool boolean);
tsc_value tsc_string(const char *str);
tsc_value tsc_lstring(const char *str, size_t len);
tsc_value tsc_cstring(const char *str);
tsc_value tsc_array(size_t len);
tsc_value tsc_object();
tsc_value tsc_cellPtr(tsc_cell *cell);
tsc_value tsc_ownedCell(tsc_cell *cell);
void tsc_retain(tsc_value value);
void tsc_destroy(tsc_value value);

void tsc_ensureArgs(tsc_value args, int min);
void tsc_varArgs(tsc_value args, int min);
void tsc_append(tsc_value list, tsc_value value);

tsc_value tsc_index(tsc_value list, size_t index);
void tsc_setIndex(tsc_value list, size_t index, tsc_value value);
tsc_value tsc_getKey(tsc_value object, const char *key);
void tsc_setKey(tsc_value object, const char *key, tsc_value value);

bool tsc_isNull(tsc_value value);
bool tsc_isInt(tsc_value value);
bool tsc_isNumber(tsc_value value);
bool tsc_isNumerical(tsc_value value);
bool tsc_isBoolean(tsc_value value);
bool tsc_isString(tsc_value value);
bool tsc_isArray(tsc_value value);
bool tsc_isObject(tsc_value value);
bool tsc_isCellPtr(tsc_value cell);
bool tsc_isOwnedCell(tsc_value cell);
bool tsc_isCell(tsc_value cell);

int64_t tsc_toInt(tsc_value value);
double tsc_toNumber(tsc_value value);
bool tsc_toBoolean(tsc_value value);
const char *tsc_toString(tsc_value value);
const char *tsc_toLString(tsc_value value, size_t *len);
size_t tsc_getLength(tsc_value value);
const char *tsc_keyAt(tsc_value object, size_t index);
tsc_cell *tsc_toCell(tsc_value value);

typedef tsc_value (tsc_signal_t)(tsc_value args);

#define TSC_VALUE_UNION 1024
#define TSC_VALUE_OPTIONAL 1025
#define TSC_VALUE_TUPLE 1026

typedef struct tsc_typeinfo_t {
    size_t tag;
    union {
        struct tsc_typeinfo_t *child;
        struct {
            struct tsc_typeinfo_t *children;
            size_t childCount;
        };
        struct {
            const char **keys;
            struct tsc_typeinfo_t *values;
            size_t pairCount;
        };
    };
} tsc_typeinfo_t;

// TODO: define typeInfo
const char *tsc_setupSignal(const char *id, tsc_signal_t *signal, tsc_typeinfo_t *typeInfo);
tsc_typeinfo_t *tsc_getSignalInfo(const char *id);
tsc_signal_t *tsc_getSignal(const char *id);
tsc_value tsc_callSignal(tsc_signal_t *signal, tsc_value *argv, size_t argc);

#endif
