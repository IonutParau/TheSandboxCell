#ifndef TSC_JSON_H
#define TSC_JSON_H

#include "value.h"
#include "../saving/saving.h"

tsc_buffer tsc_json_encode(tsc_value value, tsc_buffer *err);
tsc_value tsc_json_decode(const char *text, tsc_buffer *err);

#endif
