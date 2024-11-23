#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "value.h"
#include "api.h"

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
    string->memory = malloc(sizeof(char) * (len + 1));
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

tsc_value tsc_cellPtr(tsc_cell *cell) {
    tsc_value v;
    v.tag = TSC_VALUE_CELLPTR;
    v.cellptr = cell;
    return v;
}

tsc_value tsc_ownedCell(tsc_cell *cell) {
    tsc_value v;
    v.tag = TSC_VALUE_OWNEDCELL;
    tsc_ownedcell_t *owned = malloc(sizeof(tsc_ownedcell_t));
    owned->refc = 1;
    owned->cell = tsc_cell_clone(cell);
    v.ownedcell = owned;
    return v;
}

void tsc_retain(tsc_value value) {
    if(value.tag == TSC_VALUE_STRING) {
        value.string->refc++;
        return;
    }
    if(value.tag == TSC_VALUE_ARRAY) {
        value.array->refc++;
        return;
    }
    if(value.tag == TSC_VALUE_OBJECT) {
        value.object->refc++;
        return;
    }
    if(value.tag == TSC_VALUE_OWNEDCELL) {
        value.ownedcell->refc++;
        return;
    }
}

void tsc_destroy(tsc_value value) {
    if(value.tag == TSC_VALUE_STRING) {
        value.string->refc--;
        if(value.string->refc == 0) {
            free(value.string->memory);
            free(value.string);
        }
        return;
    }
    if(value.tag == TSC_VALUE_ARRAY) {
        value.array->refc--;
        if(value.array->refc == 0) {
            for(size_t i = 0; i < value.array->valuec; i++) {
                tsc_destroy(value.array->values[i]);
            }
            free(value.array->values);
            free(value.array);
        }
        return;
    }
    if(value.tag == TSC_VALUE_OBJECT) {
        value.object->refc--;
        if(value.object->refc == 0) {
            for(size_t i = 0; i < value.object->len; i++) {
                tsc_destroy(value.object->values[i]);
                free(value.object->keys[i]);
            }
            free(value.object->values);
            free(value.object->keys);
            free(value.object);
        }
        return;
    }
    if(value.tag == TSC_VALUE_OWNEDCELL) {
        value.ownedcell->refc--;
        if(value.ownedcell->refc == 0) {
            tsc_cell_destroy(value.ownedcell->cell);
            free(value.ownedcell);
        }
        return;
    }
}

void tsc_ensureArgs(tsc_value args, int min) {
    if(args.tag != TSC_VALUE_ARRAY) return;
    if(args.array->valuec < min) {
        args.array->values = realloc(args.array->values, sizeof(tsc_value) * min);
        for(size_t i = args.array->valuec; i < min; i++) {
            args.array->values[i] = tsc_null();
        }
        args.array->valuec = min;
    }
}

void tsc_varArgs(tsc_value args, int min) {
    if(args.tag != TSC_VALUE_ARRAY) return;
    tsc_ensureArgs(args, min+1);
    size_t len = tsc_getLength(args);
    tsc_value varargs = tsc_array(len - min);
    for(size_t i = min; i < len; i++) {
        tsc_value val = tsc_index(args, i);
        tsc_setIndex(varargs, i - min, val);
        tsc_setIndex(args, i, tsc_null());
    }
    // Unsafe delete everything else
    // This works because we deleted everything before
    args.array->valuec = min+1;
    args.array->values = realloc(args.array->values, sizeof(tsc_value) * (min + 1));
    tsc_setIndex(args, min, varargs);
    tsc_destroy(varargs);
}

void tsc_append(tsc_value list, tsc_value value) {
    if(list.tag != TSC_VALUE_ARRAY) return;
    size_t idx = list.array->valuec++;
    list.array->values = realloc(list.array->values, sizeof(tsc_value) * list.array->valuec);
    tsc_retain(value);
    list.array->values[idx] = value;
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

bool tsc_isNull(tsc_value value) {
    return value.tag == TSC_VALUE_NULL;
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

bool tsc_isCellPtr(tsc_value cell) {
    return cell.tag == TSC_VALUE_CELLPTR;
}

bool tsc_isOwnedCell(tsc_value cell) {
    return cell.tag == TSC_VALUE_OWNEDCELL;
}

bool tsc_isCell(tsc_value cell) {
    return tsc_isCellPtr(cell) || tsc_isOwnedCell(cell);
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

tsc_cell *tsc_toCell(tsc_value value) {
    if(value.tag == TSC_VALUE_CELLPTR) return value.cellptr;
    if(value.tag == TSC_VALUE_OWNEDCELL) return value.ownedcell->cell;
    return NULL;
}

static tsc_typeinfo_t *tsc_cloneTypeInfo(tsc_typeinfo_t *typeInfo) {
    tsc_typeinfo_t *copy = malloc(sizeof(tsc_typeinfo_t));
    copy->tag = typeInfo->tag;
    if(copy->tag == TSC_VALUE_ARRAY || copy->tag == TSC_VALUE_OPTIONAL) {
        copy->child = tsc_cloneTypeInfo(typeInfo->child);
    } else if(copy->tag == TSC_VALUE_TUPLE || copy->tag == TSC_VALUE_UNION) {
        copy->childCount = typeInfo->childCount;
        copy->children = malloc(sizeof(tsc_typeinfo_t) * copy->childCount);
        for(int i = 0; i < copy->childCount; i++) {
            tsc_typeinfo_t *child = tsc_cloneTypeInfo(&typeInfo->children[i]);
            copy->children[i] = *child;
            free(child);
        }
    } else if(copy->tag == TSC_VALUE_OBJECT) {
        copy->pairCount = typeInfo->pairCount;
        for(int i = 0; i < typeInfo->pairCount; i++) {
            char *key = tsc_strdup(typeInfo->keys[i]);
            copy->keys[i] = key;
            tsc_typeinfo_t *child = tsc_cloneTypeInfo(&typeInfo->children[i]);
            copy->values[i] = *child;
            free(child);
        }
    }
    return copy;
}

typedef struct tsc_signalStore {
    const char **ids;
    tsc_signal_t **signals;
    tsc_typeinfo_t **info;
    size_t len;
} tsc_signalStore;

tsc_signalStore tsc_signals = {NULL, NULL, 0};

const char *tsc_setupSignal(const char *id, tsc_signal_t *signal, tsc_typeinfo_t *typeInfo) {
    // beautiful code I know
    id = tsc_strintern(tsc_padWithModID(id));

    // Make it OWNED by US
    if(typeInfo != NULL) typeInfo = tsc_cloneTypeInfo(typeInfo);

    size_t idx = tsc_signals.len++;
    tsc_signals.ids = realloc(tsc_signals.ids, sizeof(const char *) * tsc_signals.len);
    tsc_signals.signals = realloc(tsc_signals.signals, sizeof(tsc_signal_t *) * tsc_signals.len);
    tsc_signals.info = realloc(tsc_signals.info, sizeof(tsc_typeinfo_t *) * tsc_signals.len);
    tsc_signals.ids[idx] = id;
    tsc_signals.signals[idx] = signal;
    tsc_signals.info[idx] = typeInfo;
    return id;
}

tsc_typeinfo_t *tsc_getSignalInfo(const char *id) {
    for(int i = 0; i < tsc_signals.len; i++) {
        if(tsc_signals.ids[i] == id) {
            return tsc_signals.info[i];
        }
    }
    return NULL;
}

tsc_signal_t *tsc_getSignal(const char *id) {
    for(int i = 0; i < tsc_signals.len; i++) {
        if(tsc_signals.ids[i] == id) {
            return tsc_signals.signals[i];
        }
    }
    return NULL;
}

tsc_value tsc_callSignal(tsc_signal_t *signal, tsc_value *argv, size_t argc) {
    tsc_value args = tsc_array(argc);
    for(int i = 0; i < argc; i++) {
        tsc_setIndex(args, i, argv[i]);
        tsc_destroy(argv[i]);
    }

    return signal(args);
}
