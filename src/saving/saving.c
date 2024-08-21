#include "saving.h"
#include "../utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// Damn you Nathan and your massive vault solves
#include "../threads/tinycthread.h"
#include "../threads/workers.h"
#include "../utils.h"

static tsc_saving_format *saving_arr = NULL;
static size_t savingc = 0;

int tsc_saving_encodeWith(tsc_buffer *buffer, tsc_grid *grid, const char *name) {
    for(size_t i = 0; i < savingc; i++) {
        if(strcmp(saving_arr[i].name, name) == 0) {
            if(saving_arr[i].encode == NULL) continue;
            return saving_arr[i].encode(buffer, grid);
        }
    }

    return false;
}

// A lot of heap allocations here.
void tsc_saving_encodeWithSmallest(tsc_buffer *buffer, tsc_grid *grid) {
    tsc_buffer best = tsc_saving_newBuffer(NULL);
    size_t bestSize = SIZE_MAX;

    for(size_t i = 0; i < savingc; i++) {
        if(saving_arr[i].encode == NULL) continue;
        tsc_buffer tmp = tsc_saving_newBufferCapacity(NULL, 4096);
        if(saving_arr[i].encode(&tmp, grid) == 0) {
            // Encoding failed.
            tsc_saving_deleteBuffer(tmp);
            continue;
        }
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

const char *tsc_saving_identify(const char *code) {
    for(size_t i = 0; i < savingc; i++) {
        if(strncmp(saving_arr[i].header, code, strlen(saving_arr[i].header)) == 0) {
            return saving_arr[i].name;
        }
    }
    return NULL;
}

const char *saving_base74 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$%&+-.=?^{}";

static int tsc_saving_count74(int num) {
    if(num == 0) return 1;
    int n = 0;
    while(num > 0) {
        n++;
        num = num / strlen(saving_base74);
    }
    return n;
}

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
    if(cell->data != NULL) return NULL; // Can not be stored accurately
    if(cell->texture != NULL) return NULL; // Can not be stored accurately
    if(cell->flags != 0) return NULL; // Can not be stored accurately
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
    return NULL; // Can not be stored accurately
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
        if(code[*idx] == '\0') break;
        if(code[*idx] == ';') {
            (*idx)++;
            break;
        } else {
            len++;
            (*idx)++;
        }
    }
    char *buf = malloc(sizeof(char) * (len + 1));
    buf[len] = '\0';
    memcpy(buf, mem, sizeof(char) * len);
    return buf;
}

static char *tsc_v3_nextPartUntil(const char *code, size_t *idx, char sep) {
    const char *mem = code + *idx;
    size_t len = 0;
    while(true) {
        if(code[*idx] == '\0') break;
        if(code[*idx] == sep) {
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
            tsc_buffer cellcountencoded = tsc_saving_newBuffer("");
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

            tsc_buffer repcountencoded = tsc_saving_newBuffer("");
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

static void tsc_v3_writeRepeater(tsc_buffer *buffer, int length, int back) {
    char *elen = tsc_saving_encode74(length);
    char *eback = tsc_saving_encode74(back-1);
    if(length >= 74) {
        tsc_saving_writeFormat(buffer, "(%s(%s)", eback, elen);
    } else if(back > 74) {
        tsc_saving_writeFormat(buffer, "(%s)%s", eback, elen);
    } else {
        tsc_saving_writeFormat(buffer, ")%s%s", eback, elen);
    }
    free(elen);
    free(eback);
}

static int tsc_v3_weightOfRepeat(int length, int back) {
    if(length >= 74) {
        return 3 + tsc_saving_count74(back) + tsc_saving_count74(length);
    }
    if(back > 74) {
        return 2 + tsc_saving_count74(back) + tsc_saving_count74(length);
    }
    return 1 + tsc_saving_count74(back) + tsc_saving_count74(length);
}

static int tsc_v3_encode(tsc_buffer *buffer, tsc_grid *grid) {
    tsc_saving_writeStr(buffer, "V3;");
    char *ewidth = tsc_saving_encode74(grid->width);
    char *eheight = tsc_saving_encode74(grid->height);
    tsc_saving_writeFormat(buffer, "%s;%s;", ewidth, eheight);
    free(ewidth);
    free(eheight);

    char *cells = malloc(sizeof(char) * grid->width * grid->height);
    int ci = 0;
    for(int y = grid->height-1; y >= 0; y--) {
        for(int x = 0; x < grid->width; x++) {
            tsc_cell *cell = tsc_grid_get(grid, x, y);
            bool hasbg = false;

            char *encoded = tsc_v3_celltochar(cell, hasbg);
            if(encoded == NULL) {
                free(cells);
                return 0; // V3 can't encode this
            }

            // Should ONLY be one character
            cells[ci] = *encoded;
            ci++;
            free(encoded);
        }
    }

    int cell_len = grid->width * grid->height;
    // Dispose of final empties, for compression.
    while(cells[cell_len-1] == '{') {
        cell_len--;
    }

    for(int i = 0; i < cell_len; i++) {
        int bestback = -1;
        int bestbacklen = 0;

        for(int b = 1; b < i; b++) {
            if(cells[i] == cells[i-b]) {
                int len = 1;
                while(i + len < cell_len) {
                    if(cells[i+len] == cells[i-b+len]) {
                        len++;
                    } else {
                        break;
                    }
                }
                int weight = tsc_v3_weightOfRepeat(b, len);
                if(weight > len) continue;
                if(len > bestbacklen) {
                    bestbacklen = len;
                    bestback = b;
                }
            }
        }

        // Either no copy found or the copy would not save space
        if(bestback == -1) {
            tsc_saving_write(buffer, cells[i]);
        } else {
            tsc_v3_writeRepeater(buffer, bestbacklen, bestback);
            i += bestbacklen - 1;
        }
    }

    tsc_saving_write(buffer, ';');

    tsc_saving_writeFormat(buffer, "%s;%s;", grid->title == NULL ? "" : grid->title, grid->desc == NULL ? "" : grid->desc);

    return 1; // Yo it worked!
}

void tsc_v2_decode(const char *code, tsc_grid *grid) {
    size_t index = 3;
    char *ewidth = tsc_v3_nextPart(code, &index);
    char *eheight = tsc_v3_nextPart(code, &index);

    int width = tsc_saving_decode74(ewidth);
    free(ewidth);
    int height = tsc_saving_decode74(eheight);
    free(eheight);

    tsc_clearGrid(grid, width, height);

    char *cells = tsc_v3_nextPart(code, &index);

    tsc_cell *cellArr = malloc(sizeof(tsc_cell) * width * height);
    int celli = 0;
    for(size_t i = 0; cells[i] != '\0'; i++) {
        char c = cells[i];
        char str[] = {c, '\0'};
        if(c == ')') {
            i++;
            char count[] = {cells[i], '\0'};
            int amount = tsc_saving_decode74(count);
            for(int j = 0; j < amount; j++) {
                // Unsafe copy but none of these cells have any managed memory.
                cellArr[celli] = cellArr[celli-1];
                celli++;
            }
        } else if(c == '(') {
            i++;
            char *count = tsc_v3_nextPartUntil(cells, &i, ')');
            i--;
            int amount = tsc_saving_decode74(count);
            free(count);

            for(int j = 0; j < amount; j++) {
                cellArr[celli] = cellArr[celli-1];
                celli++;
            }
        } else {
            bool isbg = false;
            cellArr[celli] = tsc_v3_chartocell(str, &isbg);
            celli++;
        }
    }

    free(cells);
    int j = 0;
    for(int y = height - 1; y >= 0; y--) {
        for(int x = 0; x < width; x++) {
            tsc_grid_set(grid, x, y, &cellArr[j]);
            j++;
            if(j == celli) goto ohno;
        }
    }
ohno:
    free(cellArr);

    char *title = tsc_v3_nextPart(code, &index);
    grid->title = tsc_strintern(title);
    free(title);
    
    char *desc = tsc_v3_nextPart(code, &index);
    grid->desc = tsc_strintern(desc);
    free(desc);
}

void tsc_v1_decode(const char *code, tsc_grid *grid) {
    size_t index = 3;
    char *ewidth = tsc_v3_nextPart(code, &index);
    char *eheight = tsc_v3_nextPart(code, &index);

    int width = atoi(ewidth);
    int height = atoi(eheight);

    tsc_clearGrid(grid, width, height);

    free(ewidth);
    free(eheight);

    char *placeables = tsc_v3_nextPart(code, &index);
    // TODO: placeables
    free(placeables);

    const char *ids[] = {
        builtin.generator, builtin.rotator_cw, builtin.rotator_ccw,
        builtin.mover, builtin.slide, builtin.push, builtin.wall,
        builtin.enemy, builtin.trash,
    };
    size_t idc = sizeof(ids) / sizeof(const char *);

    char *cells = tsc_v3_nextPart(code, &index);
    size_t celli = 0;
    while(cells[celli] != '\0') {
        char *celldata = tsc_v3_nextPartUntil(cells, &celli, ',');
        size_t celldatai = 0;

        // we free ASAP for performance.
        // This allows the free list to reuse memory for us automatically
        // If you think this code isn't readable,
        // I promise you, you haven't seen shit yet.

        char *strid = tsc_v3_nextPartUntil(celldata, &celldatai, '.');
        int ididx = atoi(strid);
        free(strid);

        const char *id = ididx < idc ? ids[ididx] : builtin.empty;
        
        char *strrot = tsc_v3_nextPartUntil(celldata, &celldatai, '.');
        int rot = atoi(strrot);
        rot %= 4;
        free(strrot);
        
        char *strx = tsc_v3_nextPartUntil(celldata, &celldatai, '.');
        int x = atoi(strx);
        free(strx);
        
        char *stry = tsc_v3_nextPartUntil(celldata, &celldatai, '.');
        int y = height - atoi(stry) - 1;
        free(stry);

        tsc_cell cell = tsc_cell_create(id, rot);
        tsc_grid_set(grid, x, y, &cell);

        free(celldata);
    }
    free(cells);

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
    v3.encode = tsc_v3_encode;
    tsc_saving_register(v3);

    tsc_saving_format v2 = {};
    v2.name = "V2";
    v2.header = "V2;";
    v2.decode = tsc_v2_decode;
    v2.encode = NULL;
    tsc_saving_register(v2);
    
    tsc_saving_format v1 = {};
    v1.name = "V1";
    v1.header = "V1;";
    v1.decode = tsc_v1_decode;
    v1.encode = NULL;
    tsc_saving_register(v1);
}
