#include "saving.h"
#include "../utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static tsc_saving_format *saving_arr = NULL;
static size_t savingc = 0;

int tsc_saving_encodeWith(tsc_saving_buffer *buffer, tsc_grid *grid, const char *name) {
    for(size_t i = 0; i < savingc; i++) {
        if(strcmp(saving_arr[i].name, name) == 0) {
            if(saving_arr[i].encode == NULL) continue;
            return saving_arr[i].encode(buffer, grid);
        }
    }

    return false;
}

// A lot of heap allocations here.
void tsc_saving_encodeWithSmallest(tsc_saving_buffer *buffer, tsc_grid *grid) {
    tsc_saving_buffer best = tsc_saving_newBuffer(NULL);
    size_t bestSize = SIZE_MAX;

    for(size_t i = 0; i < savingc; i++) {
        if(saving_arr[i].encode == NULL) continue;
        tsc_saving_buffer tmp = tsc_saving_newBuffer(NULL);
        saving_arr[i].encode(&tmp, grid);
        if(tmp.len < bestSize) {
            bestSize = tmp.len;
            tsc_saving_deleteBuffer(best);
            best = tsc_saving_newBuffer(NULL);
            tsc_saving_writeBytes(&best, tmp.mem, tmp.len);
        }
        tsc_saving_deleteBuffer(tmp);
    }

    tsc_saving_writeBytes(buffer, best.mem, best.len);
    tsc_saving_deleteBuffer(best);
}

void tsc_saving_decodeWith(const char *code, tsc_grid *grid, const char *name) {
    for(size_t i = 0; i < savingc; i++) {
        if(strcmp(saving_arr[i].name, name) == 0) {
            if(saving_arr[i].decode == NULL) continue;
            saving_arr[i].decode(code, grid);
            return;
        }
    }
}

void tsc_saving_decodeWithAny(const char *code, tsc_grid *grid) {
    for(size_t i = 0; i < savingc; i++) {
        if(strncmp(saving_arr[i].header, code, strlen(saving_arr[i].header)) == 0) {
            if(saving_arr[i].decode == NULL) continue;
            saving_arr[i].decode(code, grid);
            return;
        }
    }
}

const char *saving_base74 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$%&+-.=?^{}";

static char *tsc_saving_encode74(int num) {
    if(num == 0) {
        return tsc_strdup("0");
    }
    char *result = tsc_strdup("");
    size_t resc = 0;
    while(num > 0) {
        int n = num % strlen(saving_base74);
        num = num / strlen(saving_base74);
        char *old = result;
        result = malloc(sizeof(char) * (resc + 2));
        result[0] = saving_base74[n];
        strcpy(result + 1, old);
        free(old);
        resc++;
    }
    return result;
}

static int tsc_saving_decode74(const char *num) {
    int n = 0;

    int numc = strlen(num);

    for(int i = 0; i < numc; i++) {
        char c = num[i];
        int val;
        for(int j = 0; saving_base74[j] != '\0'; j++) {
            if(saving_base74[j] == c) {
                val = j;
                break;
            }
        }
        n *= strlen(saving_base74);
        n += val;
    }

    return n;
}

// Caller owns memory
// NULL if conversion fails (V3 does not support the cell)
static char *tsc_v3_celltochar(tsc_cell *cell, bool hasBg) {
    if(cell->data == NULL) return NULL; // Can not be stored accurately
    if(cell->texture == NULL) return NULL; // Can not be stored accurately
    if(cell->id == builtin.empty) {
        return tsc_saving_encode74(72 + (hasBg ? 1 : 0));
    }
    const char *ids[] = {
        builtin.generator, builtin.rotator_cw,
        builtin.rotator_ccw, builtin.mover,
        builtin.slide, builtin.push,
        builtin.wall, builtin.enemy,
        builtin.trash,
    };
    int idc = sizeof(ids) / sizeof(char *);
    int id = 0;
    for(int i = 0; i < idc; i++) {
        if(ids[i] == cell->id) {
            id = i;
            goto success;
        }
    }
success:
    return tsc_saving_encode74(id * 2 + (hasBg ? 1 : 0) + ((int)cell->rot) * 9 * 2);
}

static tsc_cell tsc_v3_chartocell(const char *input, bool *hasBg) {
    int n = tsc_saving_decode74(input);
    *hasBg = (n % 2 == 1);

    if(n > 71) {
        return tsc_cell_create(builtin.empty, 0);
    }
    
    const char *ids[] = {
        builtin.generator, builtin.rotator_cw,
        builtin.rotator_ccw, builtin.mover,
        builtin.slide, builtin.push,
        builtin.wall, builtin.enemy,
        builtin.trash,
    };

    int val = n % (9 * 2);
    char rot = n / (9 * 2);
    
    const char *id = ids[val / 2];

    return tsc_cell_create(id, rot);
}

static char *tsc_v3_nextPart(const char *code, size_t *idx) {
    const char *mem = code + *idx;
    size_t len = 0;
    while(true) {
        if(code[*idx] == ';') {
            (*idx)++;
            break;
        } else {
            len++;
            (*idx)++;
        }
    }
    if(len == 0) return NULL;
    char *buf = malloc(sizeof(char) * (len + 1));
    buf[len] = '\0';
    memcpy(buf, mem, sizeof(char) * len);
    return buf;
}

static void tsc_v3_decode(const char *code, tsc_grid *grid) {
    size_t index = 3;
    char *swidth = tsc_v3_nextPart(code, &index);
    char *sheight = tsc_v3_nextPart(code, &index);
    int width = tsc_saving_decode74(swidth);
    int height = tsc_saving_decode74(sheight);
    free(swidth);
    free(sheight);
    tsc_clearGrid(grid, width, height);

    char *cells = tsc_v3_nextPart(code, &index);
    printf("%s\n", cells);
    tsc_cell *cellArr = malloc(sizeof(tsc_cell) * width * height);
    size_t celli = 0;
    for(int i = 0; cells[i] != '\0'; i++) {
        char c = cells[i];
        char str[2] = {c, '\0'};
        if(c == ')') {
            char scellcount[2] = {cells[i+1], '\0'};
            char srepcount[2] = {cells[i+2], '\0'};
            int cellcount = tsc_saving_decode74(scellcount)+1;
            int repcount = tsc_saving_decode74(srepcount);
            i += 2;

            for(size_t j = 0; j < repcount; j++) {
                cellArr[celli] = cellArr[celli-cellcount];
                celli++;
            }
        } else if(c == '(') {
            bool simplerepcount = false;
            i++;
            tsc_saving_buffer cellcountencoded = tsc_saving_newBuffer("");
            while(true) {
                if(cells[i] == '(') {
                    break;
                } else if(cells[i] == ')') {
                    simplerepcount = true;
                    break;
                } else {
                    tsc_saving_write(&cellcountencoded, cells[i]);
                    i++;
                }
            }

            int cellcount = tsc_saving_decode74(cellcountencoded.mem)+1;
            tsc_saving_deleteBuffer(cellcountencoded);

            tsc_saving_buffer repcountencoded = tsc_saving_newBuffer("");
            i++;
            if(simplerepcount) {
                tsc_saving_write(&repcountencoded, cells[i]);
            } else {
                while(true) {
                    if(cells[i] == ')') {
                        break;
                    } else {
                        tsc_saving_write(&repcountencoded, cells[i]);
                        i++;
                    }
                }
            }

            int repcount = tsc_saving_decode74(repcountencoded.mem);
            tsc_saving_deleteBuffer(repcountencoded);

            for(size_t j = 0; j < repcount; j++) {
                cellArr[celli] = cellArr[celli-cellcount];
                celli++;
            }
        } else {
            bool isBg = false;
            tsc_cell cell = tsc_v3_chartocell(str, &isBg);
            cellArr[celli] = cell;
            celli++;
        }
    }

    // Stupid V3 encoding lets you omit cells
    while(celli < width * height) {
        cellArr[celli++] = tsc_cell_create(builtin.empty, 0);
    }

    celli = 0;
    for(int y = height-1; y >= 0; y--) {
        for(int x = 0; x < width; x++) {
            tsc_grid_set(grid, x, y, &cellArr[celli]);
            celli++;
        }
    }

    char *title = tsc_v3_nextPart(code, &index);
    grid->title = tsc_strintern(title);
    free(title);
    
    char *desc = tsc_v3_nextPart(code, &index);
    grid->desc = tsc_strintern(desc);
    free(desc);
}

void tsc_saving_register(tsc_saving_format format) {
    size_t idx = savingc++;
    saving_arr = realloc(saving_arr, sizeof(tsc_saving_format) * savingc);
    saving_arr[idx] = format;
}

void tsc_saving_registerCore() {
    tsc_saving_format v3 = {};
    v3.name = "V3";
    v3.header = "V3;";
    v3.decode = tsc_v3_decode;
    v3.encode = NULL;
    tsc_saving_register(v3);
}
