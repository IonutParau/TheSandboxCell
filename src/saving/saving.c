#include "saving.h"
#include "../utils.h"
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../utils.h"
#include "../api/value.h"
#include "../api/api.h"
#include "../threads/workers.h"
#include <raylib.h>
#include <assert.h>

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
        if(saving_arr[i].flags & TSC_SAVING_COMPATIBILITY) continue; // encoder considered not ideal for standard usage
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
int saving_base74R[256] = {-1};

static int tsc_saving_count74(int num) {
    if(num == 0) return 1;
    int n = 0;
    while(num > 0) {
        n++;
        num = num / 74;
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
        int n = num % 74;
        num = num / 74;
        char *old = result;
        result = malloc(sizeof(char) * (resc + 2));
        result[0] = saving_base74[n];
        strcpy(result + 1, old);
        free(old);
        resc++;
    }
    return result;
}

static char tsc_saving_encodeChar74(char n) {
    assert(n < 74);
    return saving_base74[(size_t)n];
}

static int tsc_saving_decode74(const char *num) {
    int n = 0;

    for(int i = 0; num[i] != '\0'; i++) {
        char c = num[i];
        int val = 0;
        for(int j = 0; j < 74; j++) {
            if(saving_base74[j] == c) {
                val = j;
                break;
            }
        }
        n *= 74;
        n += val;
    }

    return n;
}

static char tsc_saving_decodeChar74(char c) {
    if(saving_base74R[0] == -1) {
        for(int i = 0; i < 74; i++) {
            saving_base74R[(size_t)tsc_saving_encodeChar74(i)] = i;
        }
        saving_base74R[0] = 0;
    }

    return saving_base74R[(size_t)c];
}


// Caller owns memory
// NULL if conversion fails (V3 does not support the cell)
static char tsc_v3_celltochar(tsc_cell *cell, bool hasBg) {
#ifndef TSC_TURBO
    if(cell->texture != TSC_NULL_TEXTURE) return '\0'; // Can not be stored accurately
    if(cell->effect != TSC_NULL_EFFECT) return '\0'; // Can not be stored accurately
    if(cell->reg != TSC_NULL_REGISTRY) return '\0'; // Can not be stored accurately
#endif
    if(cell->id == builtin.empty) {
        return tsc_saving_encodeChar74(72 + (hasBg ? 1 : 0));
    }
    tsc_id_t ids[] = {
        builtin.generator, builtin.rotator_cw,
        builtin.rotator_ccw, builtin.mover,
        builtin.slide, builtin.push,
        builtin.wall, builtin.enemy,
        builtin.trash,
    };
    int idc = sizeof(ids) / sizeof(ids[0]);
    int id = 0;
    for(int i = 0; i < idc; i++) {
        if(ids[i] == cell->id) {
            id = i;
            goto success;
        }
    }
    return '\0'; // Can not be stored accurately
success:
    return tsc_saving_encodeChar74(id * 2 + (hasBg ? 1 : 0) + ((int)cell->rotData & 0b11) * 9 * 2);
}

static tsc_cell tsc_v3_chartocell(char input, bool *hasBg) {
    int n = tsc_saving_decodeChar74(input);
    *hasBg = (n % 2 == 1);

    if(n > 71) {
        return tsc_cell_create(builtin.empty, 0);
    }

    tsc_id_t ids[] = {
        builtin.generator, builtin.rotator_cw,
        builtin.rotator_ccw, builtin.mover,
        builtin.slide, builtin.push,
        builtin.wall, builtin.enemy,
        builtin.trash,
    };

    int val = n % (9 * 2);
    char rot = n / (9 * 2);

    tsc_id_t id = ids[val / 2];

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
    bool *bgArr = malloc(sizeof(bool) * width * height);
    size_t celli = 0;
    for(int i = 0; cells[i] != '\0'; i++) {
        char c = cells[i];
        if(c == ')') {
            int cellcount = tsc_saving_decodeChar74(cells[i+1])+1;
            int repcount = tsc_saving_decodeChar74(cells[i+2]);
            i += 2;

            for(size_t j = 0; j < repcount; j++) {
                cellArr[celli] = cellArr[celli-cellcount];
                bgArr[celli] = bgArr[celli-cellcount];
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
                bgArr[celli] = bgArr[celli-cellcount];
                celli++;
            }
        } else {
            bool isBg = false;
            tsc_cell cell = tsc_v3_chartocell(c, &isBg);
            cellArr[celli] = cell;
            bgArr[celli] = isBg;
            celli++;
        }
    }

    // Stupid V3 encoding lets you omit cells
    while(celli < width * height) {
        cellArr[celli] = tsc_cell_create(builtin.empty, 0);
        bgArr[celli] = false;
        celli++;
    }

    celli = 0;
    tsc_cell place = tsc_cell_create(builtin.placeable, 0);
    for(int y = height-1; y >= 0; y--) {
        for(int x = 0; x < width; x++) {
            tsc_grid_set(grid, x, y, &cellArr[celli]);
            if(bgArr[celli]) {
                tsc_grid_setBackground(grid, x, y, &place);
            }
            celli++;
        }
    }

    free(cellArr);
    free(bgArr);

    char *title = tsc_v3_nextPart(code, &index);
    grid->title = strlen(title) == 0 ? NULL : tsc_strintern(title);
    free(title);

    char *desc = tsc_v3_nextPart(code, &index);
    grid->desc = strlen(desc) == 0 ? NULL : tsc_strintern(desc);
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
            bool hasbg = tsc_grid_background(grid, x,y)->id != builtin.empty;

            char encoded = tsc_v3_celltochar(cell, hasbg);
            if(encoded == '\0') {
                free(cells);
                return 0; // V3 can't encode this
            }

            cells[ci] = encoded;
            ci++;
        }
    }

    int cell_len = grid->width * grid->height;
    // Dispose of final empties, for compression.
    while(cells[cell_len-1] == '{') {
        cell_len--;
    }

    int speed = atoi(tsc_toString(tsc_getSetting(builtin.settings.v3speed)));
    int maxWork = cell_len / (speed + 1);

    for(int i = 0; i < cell_len; i++) {
        int bestback = -1;
        int bestbacklen = 0;

        for(int b = 1; b <= i && b <= maxWork; b++) {
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
    bool *bgArr = malloc(sizeof(bool) * width * height);
    int celli = 0;
    for(size_t i = 0; cells[i] != '\0'; i++) {
        char c = cells[i];
        if(c == ')') {
            i++;
            int amount = tsc_saving_decodeChar74(cells[i]);
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
                bgArr[celli] = bgArr[celli-1];
                celli++;
            }
        } else {
            bool isbg = false;
            cellArr[celli] = tsc_v3_chartocell(c, &isbg);
            bgArr[celli] = isbg;
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
    free(bgArr);

    char *title = tsc_v3_nextPart(code, &index);
    grid->title = strlen(title) == 0 ? NULL : tsc_strintern(title);
    free(title);

    char *desc = tsc_v3_nextPart(code, &index);
    grid->desc = strlen(desc) == 0 ? NULL : tsc_strintern(desc);
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

    tsc_id_t ids[] = {
        builtin.generator, builtin.rotator_cw, builtin.rotator_ccw,
        builtin.mover, builtin.slide, builtin.push, builtin.wall,
        builtin.enemy, builtin.trash,
    };
    size_t idc = sizeof(ids) / sizeof(ids[0]);

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

        tsc_id_t id = ididx < idc ? ids[ididx] : builtin.empty;

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
    grid->title = strlen(title) == 0 ? NULL : tsc_strintern(title);
    free(title);

    char *desc = tsc_v3_nextPart(code, &index);
    grid->desc = strlen(desc) == 0 ? NULL : tsc_strintern(desc);
    free(desc);
}

typedef struct tsc_tsc_state {
    char headerByte;
    // returns 0 on failure
    unsigned int (*encoder)(tsc_buffer *buffer, tsc_grid *grid, int idx, int len);
    // never fail
    size_t (*decoder)(tsc_grid *grid, char *data, size_t *idx);
} tsc_tsc_state;

static unsigned int tsc_tsc_encodeEmpties(tsc_buffer *buffer, tsc_grid *grid, int idx, int len, int numSize, tsc_id_t bg) {
    size_t maximum = 1;
    maximum <<= numSize*8;

    // technically int wouldn't overflow but eh its nice to use protection
    size_t counted = 0;

    while(counted < len) {
        if(grid->bgs[idx+counted].id != bg) break; // not representable
        if(grid->cells[idx+counted].id != builtin.empty) break; // not an empty
        counted++;
    }

    if(counted > maximum) return 0;
    if(counted == 0) return 0;

    size_t toEncode = counted - 1;
    char *toEncodeIn = tsc_saving_reserveFor(buffer, numSize);
    for(int i = 0; i < numSize; i++) {
        size_t val = (toEncode >> (i*8)) & 0xFF;
        toEncodeIn[i] = val;
    }

    return counted;
}

size_t tsc_tsc_decodeEmpties(tsc_grid *grid, volatile char *data, size_t *cellIdx, int numSize, tsc_id_t bg) {
    size_t counted = 0;
    for(int i = 0; i < numSize; i++) {
        size_t dataByte = (unsigned char)data[i];
        counted |= (dataByte << (i*8));
    }
    counted++;
    for(size_t i = 0; i < counted; i++) {
        size_t j = (*cellIdx) + i;
        grid->bgs[j] = tsc_cell_create(bg, 0);
        grid->cells[j] = tsc_cell_create(builtin.empty, 0);
        if(bg != builtin.empty) tsc_grid_enableChunk(grid, j % grid->width, j / grid->width);
    }
    (*cellIdx) += counted;
    return numSize;
}

static unsigned int tsc_tsc_encodeEmpties1(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 1, builtin.empty);
}

static size_t tsc_tsc_decodeEmpties1(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 1, builtin.empty);
}

static unsigned int tsc_tsc_encodeEmpties2(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 2, builtin.empty);
}

static size_t tsc_tsc_decodeEmpties2(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 2, builtin.empty);
}

static unsigned int tsc_tsc_encodeEmpties3(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 3, builtin.empty);
}

static size_t tsc_tsc_decodeEmpties3(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 3, builtin.empty);
}

static unsigned int tsc_tsc_encodeEmpties4(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 4, builtin.empty);
}

static size_t tsc_tsc_decodeEmpties4(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 4, builtin.empty);
}

static unsigned int tsc_tsc_encodePlaces1(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 1, builtin.placeable);
}

static size_t tsc_tsc_decodePlaces1(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 1, builtin.placeable);
}

static unsigned int tsc_tsc_encodePlaces2(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 2, builtin.placeable);
}

static size_t tsc_tsc_decodePlaces2(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 2, builtin.placeable);
}

static unsigned int tsc_tsc_encodePlaces3(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 3, builtin.placeable);
}

static size_t tsc_tsc_decodePlaces3(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 3, builtin.placeable);
}

static unsigned int tsc_tsc_encodePlaces4(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeEmpties(buffer, grid, idx, len, 4, builtin.placeable);
}

static size_t tsc_tsc_decodePlaces4(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeEmpties(grid, data, cellIdx, 4, builtin.placeable);
}

typedef struct tsc_tsc_vanillaTableEntry {
    char binary;
    tsc_id_t id;
    unsigned char rot;
} tsc_tsc_vanillaTableEntry;

static unsigned int tsc_tsc_encodeVanillaBigBrainOpt(tsc_buffer *buffer, tsc_grid *grid, int idx, int len, tsc_id_t bg, int numSize) {
    unsigned int cellCount = 0;
    size_t maximum = 1;
    maximum <<= numSize*8;

    tsc_tsc_vanillaTableEntry table[16] = {
        // Super optimized 4-bit encoding
        // This simplifies cells, which technically makes the TSC format not lossless,
        // but this *SHOULD NEVER* cause issues. If it does, your mods are dogshit
        {0b0000, builtin.generator, 0},
        {0b0001, builtin.generator, 1},
        {0b0010, builtin.generator, 2},
        {0b0011, builtin.generator, 3},
        {0b0100, builtin.mover, 0},
        {0b0101, builtin.mover, 1},
        {0b0110, builtin.mover, 2},
        {0b0111, builtin.mover, 3},
        {0b1000, builtin.slide, 0},
        {0b1001, builtin.slide, 1},
        {0b1010, builtin.push, 0},
        {0b1011, builtin.wall, 0},
        {0b1100, builtin.enemy, 0},
        {0b1101, builtin.trash, 0},
        {0b1110, builtin.rotator_cw, 0},
        {0b1111, builtin.rotator_ccw, 0},
    };

    while(cellCount < len) {
        int cellIdx = idx + cellCount;

        if(grid->bgs[cellIdx].id != bg) break;

        int bin = 16;
        tsc_cell toEncode = grid->cells[cellIdx];
        toEncode.rotData &= 0b11;
        if(toEncode.id == builtin.push) toEncode.rotData = 0;
        if(toEncode.id == builtin.wall) toEncode.rotData = 0;
        if(toEncode.id == builtin.enemy) toEncode.rotData = 0;
        if(toEncode.id == builtin.trash) toEncode.rotData = 0;
        if(toEncode.id == builtin.rotator_cw) toEncode.rotData = 0;
        if(toEncode.id == builtin.rotator_ccw) toEncode.rotData = 0;
        if(toEncode.id == builtin.slide) toEncode.rotData = toEncode.rotData & 1;

        for(int i = 0; i < 16; i++) {
            if(table[i].id == toEncode.id && table[i].rot == toEncode.rotData) {
                cellCount++;
            }
        }

        if(bin == 16) break;
    }

    if(cellCount > maximum) return 0;

    // write len (little endian int)
    for(int i = 0; i < numSize; i++) {
        tsc_saving_write(buffer, (cellCount >> (i * 8)) & 0xFF);
    }

    int byteLen = cellCount / 2;
    if(cellCount % 2 != 0) byteLen++;

    char *rawBytes = malloc(byteLen);
    memset(rawBytes, 0, byteLen);

    for(int j = 0; j < cellCount; j++) {
        int cellIdx = idx + j;

        if(grid->bgs[cellIdx].id != bg) break;

        int bin = 16;
        tsc_cell toEncode = grid->cells[cellIdx];
        toEncode.rotData &= 0b11;
        if(toEncode.id == builtin.push) toEncode.rotData = 0;
        if(toEncode.id == builtin.wall) toEncode.rotData = 0;
        if(toEncode.id == builtin.enemy) toEncode.rotData = 0;
        if(toEncode.id == builtin.trash) toEncode.rotData = 0;
        if(toEncode.id == builtin.rotator_cw) toEncode.rotData = 0;
        if(toEncode.id == builtin.rotator_ccw) toEncode.rotData = 0;
        if(toEncode.id == builtin.slide) toEncode.rotData &= 1;

        for(int i = 0; i < 16; i++) {
            if(table[i].id == toEncode.id && table[i].rot == toEncode.rotData) {
                bin = i;
                break;
            }
        }

        rawBytes[j / 2] |= (bin << ((j % 2) * 4));
    }

    tsc_saving_writeBytes(buffer, rawBytes, byteLen);

    free(rawBytes);

    return cellCount;
}

static size_t tsc_tsc_decodeVanillaBigBrainOpt(tsc_grid *grid, char *data, size_t *cellIdx, tsc_id_t bg, int numSize) {
    size_t cellCount = 0;
    for(int i = 0; i < numSize; i++) {
        size_t dataByte = (unsigned char)data[i];
        cellCount |= (dataByte << (i * 8));
    }
    
    tsc_tsc_vanillaTableEntry table[16] = {
        // Super optimized 4-bit encoding
        // This simplifies cells, which technically makes the TSC format not lossless,
        // but this *SHOULD NEVER* cause issues. If it does, your mods are dogshit
        {0b0000, builtin.generator, 0},
        {0b0001, builtin.generator, 1},
        {0b0010, builtin.generator, 2},
        {0b0011, builtin.generator, 3},
        {0b0100, builtin.mover, 0},
        {0b0101, builtin.mover, 1},
        {0b0110, builtin.mover, 2},
        {0b0111, builtin.mover, 3},
        {0b1000, builtin.slide, 0},
        {0b1001, builtin.slide, 1},
        {0b1010, builtin.push, 0},
        {0b1011, builtin.wall, 0},
        {0b1100, builtin.enemy, 0},
        {0b1101, builtin.trash, 0},
        {0b1110, builtin.rotator_cw, 0},
        {0b1111, builtin.rotator_ccw, 0},
    };

    data += numSize;

    size_t byteLen = cellCount / 2 + (cellCount & 1);
    for(size_t i = 0; i < cellCount; i++) {
        char cellBits = (data[i / 2] >> ((i % 2) * 4)) & 0xF;
        size_t j = (*cellIdx) + i;
        grid->bgs[j] = tsc_cell_create(bg, 0);
        tsc_tsc_vanillaTableEntry entry = table[cellBits];
        grid->cells[j] = tsc_cell_create(entry.id, entry.rot);
        tsc_grid_enableChunk(grid, j % grid->width, j / grid->width);
    }

    (*cellIdx) += cellCount;
    return numSize + byteLen;
}

static unsigned int tsc_tsc_encodeVanillaNoPlace1(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.empty, 1);
}

static unsigned int tsc_tsc_encodeVanillaWithPlace1(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.placeable, 1);
}

static size_t tsc_tsc_decodeVanillaNoPlace1(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.empty, 1);
}

static size_t tsc_tsc_decodeVanillaWithPlace1(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.placeable, 1);
}

static unsigned int tsc_tsc_encodeVanillaNoPlace2(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.empty, 2);
}

static unsigned int tsc_tsc_encodeVanillaWithPlace2(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.placeable, 2);
}

static size_t tsc_tsc_decodeVanillaNoPlace2(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.empty, 2);
}

static size_t tsc_tsc_decodeVanillaWithPlace2(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.placeable, 2);
}

static unsigned int tsc_tsc_encodeVanillaNoPlace3(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.empty, 3);
}

static unsigned int tsc_tsc_encodeVanillaWithPlace3(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.placeable, 3);
}

static size_t tsc_tsc_decodeVanillaNoPlace3(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.empty, 3);
}

static size_t tsc_tsc_decodeVanillaWithPlace3(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.placeable, 3);
}

static unsigned int tsc_tsc_encodeVanillaNoPlace4(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.empty, 4);
}

static unsigned int tsc_tsc_encodeVanillaWithPlace4(tsc_buffer *buffer, tsc_grid *grid, int idx, int len) {
    return tsc_tsc_encodeVanillaBigBrainOpt(buffer, grid, idx, len, builtin.placeable, 4);
}

static size_t tsc_tsc_decodeVanillaNoPlace4(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.empty, 4);
}

static size_t tsc_tsc_decodeVanillaWithPlace4(tsc_grid *grid, char *data, size_t *cellIdx) {
    return tsc_tsc_decodeVanillaBigBrainOpt(grid, data, cellIdx, builtin.placeable, 4);
}

#define TSC_TSC_STATECOUNT 16
static tsc_tsc_state tsc_tsc_states[TSC_TSC_STATECOUNT] = {
    {'A', tsc_tsc_encodeEmpties1, tsc_tsc_decodeEmpties1},
    {'B', tsc_tsc_encodeEmpties2, tsc_tsc_decodeEmpties2},
    {'C', tsc_tsc_encodeEmpties3, tsc_tsc_decodeEmpties3},
    {'D', tsc_tsc_encodeEmpties4, tsc_tsc_decodeEmpties4},
    {'1', tsc_tsc_encodePlaces1, tsc_tsc_decodePlaces1},
    {'2', tsc_tsc_encodePlaces2, tsc_tsc_decodePlaces2},
    {'3', tsc_tsc_encodePlaces3, tsc_tsc_decodePlaces3},
    {'4', tsc_tsc_encodePlaces4, tsc_tsc_decodePlaces4},
    {'E', tsc_tsc_encodeVanillaNoPlace1, tsc_tsc_decodeVanillaNoPlace1},
    {'F', tsc_tsc_encodeVanillaWithPlace1, tsc_tsc_decodeVanillaWithPlace1},
    {'G', tsc_tsc_encodeVanillaNoPlace2, tsc_tsc_decodeVanillaNoPlace2},
    {'H', tsc_tsc_encodeVanillaWithPlace2, tsc_tsc_decodeVanillaWithPlace2},
    {'I', tsc_tsc_encodeVanillaNoPlace3, tsc_tsc_decodeVanillaNoPlace3},
    {'J', tsc_tsc_encodeVanillaWithPlace3, tsc_tsc_decodeVanillaWithPlace3},
    {'K', tsc_tsc_encodeVanillaNoPlace4, tsc_tsc_decodeVanillaNoPlace4},
    {'L', tsc_tsc_encodeVanillaWithPlace4, tsc_tsc_decodeVanillaWithPlace4},
};

typedef struct tsc_tsc_chunk {
    tsc_grid *grid;
    tsc_buffer buffer;
    int start;
    int len;
} tsc_tsc_chunk;

void tsc_tsc_encodeChunk(tsc_tsc_chunk *chunk) {
    tsc_buffer buffers[TSC_TSC_STATECOUNT];
    for(int i = 0; i < TSC_TSC_STATECOUNT; i++) {
        buffers[i] = tsc_saving_newBuffer(NULL);
    }

    int off = 0;
    while(off < chunk->len) {
        bool worked = false;
        for(int i = 0; i < TSC_TSC_STATECOUNT; i++) {
            tsc_tsc_state state = tsc_tsc_states[i];
            tsc_buffer *stateBuffer = buffers + i;
            tsc_saving_clearBuffer(stateBuffer);

            unsigned int encoded = state.encoder(stateBuffer, chunk->grid, chunk->start + off, chunk->len - off);
            if(encoded == 0) continue; // we failed, L
            tsc_saving_write(&chunk->buffer, state.headerByte);
            tsc_saving_writeBytes(&chunk->buffer, stateBuffer->mem, stateBuffer->len);
            off += encoded;
            worked = true;
            break; // we got a working state
        }
        if(!worked) {
            tsc_saving_clearBuffer(&chunk->buffer); // all states failed
            break;
        }
    }
    
    for(int i = 0; i < TSC_TSC_STATECOUNT; i++) {
        tsc_saving_deleteBuffer(buffers[i]);
    }
}

// best name in all of coding
int tsc_tsc_encode(tsc_buffer *buffer, tsc_grid *grid) {
    tsc_saving_writeStr(buffer, "TSC;");

    char *ewidth = tsc_saving_encode74(grid->width);
    char *eheight = tsc_saving_encode74(grid->height);
    tsc_saving_writeFormat(buffer, "%s;%s;", ewidth, eheight);
    free(ewidth);
    free(eheight);

    int minThreadWork = 10000;
    int threadCount = workers_amount();
    if(threadCount < 1) threadCount = 1;

    int whatTheFuckDidYouDo = minThreadWork * threadCount;
    int area = grid->width * grid->height;

    int perChunk;

    if(area >= whatTheFuckDidYouDo) {
        perChunk = area / threadCount;
    } else {
        perChunk = minThreadWork;
    }
        
    // We do it like this to help compression
    int maxChunkC = 1 + area / perChunk;
    int chunkc = 0;
    tsc_tsc_chunk *chunks = malloc(sizeof(chunks[0]) * maxChunkC);

    int consumed = 0;
    while(consumed < area) {
        int remaining = area - consumed;

        int len = remaining;
        if(len > perChunk) len = perChunk;

        tsc_tsc_chunk chunk = {grid, tsc_saving_newBufferCapacity(NULL, 8192), consumed, len};
        chunks[chunkc] = chunk;
        chunkc++;
        consumed += len;
    }

    workers_waitForTasksFlat((worker_task_t *)&tsc_tsc_encodeChunk, chunks, sizeof(tsc_tsc_chunk), chunkc);

    tsc_saving_buffer finalData = tsc_saving_newBuffer(NULL);

    bool failed = 0;

    for(int i = 0; i < chunkc; i++) {
        tsc_saving_writeBytes(&finalData, chunks[i].buffer.mem, chunks[i].buffer.len);
        if(chunks[i].buffer.len == 0) failed = 1;
        tsc_saving_deleteBuffer(chunks[i].buffer);
    }
    
    free(chunks);

    if(failed) {
        fprintf(stderr, "TSC format somehow failed. Bad mod?\n");
        return 0; // we failed.... somehow
    }

    int deflatedLen;
    unsigned char *deflated = CompressData((unsigned char *)finalData.mem, finalData.len, &deflatedLen); // get deflated

    int base64Len;
    char *based64 = EncodeDataBase64(deflated, deflatedLen, &base64Len);
    RL_FREE(deflated);

    tsc_saving_writeBytes(buffer, based64, base64Len);

    RL_FREE(based64);
    
    tsc_saving_write(buffer, ';');

    return 1;
}

size_t tsc_tsc_decodeChunk(tsc_grid *grid, size_t *cellIdx, char *data, char format) {
    for(size_t i = 0; i < TSC_TSC_STATECOUNT; i++) {
        tsc_tsc_state state = tsc_tsc_states[i];
        if(state.headerByte == format) {
            return state.decoder(grid, data, cellIdx);
        }
    }
    return 0; // unsupported, REALLY BAD
}

void tsc_tsc_decode(const char *code, tsc_grid *grid) {
    size_t index = 4; // 4 is after the first ;, and thus after the header

    char *ewidth = tsc_v3_nextPart(code, &index);
    char *eheight = tsc_v3_nextPart(code, &index);

    int width = tsc_saving_decode74(ewidth);
    int height = tsc_saving_decode74(eheight);

    free(ewidth);
    free(eheight);
    
    tsc_clearGrid(grid, width, height);

    size_t cellIdx = 0;
    size_t area = width * height;

    clock_t start = clock();
    unsigned char *eBase64 = (void *)tsc_v3_nextPart(code, &index);
    int deflatedLen;
    unsigned char *deflated = DecodeDataBase64(eBase64, &deflatedLen);

    int encodedLen;
    char *encodedData = (void *)DecompressData(deflated, deflatedLen, &encodedLen);
    char *startOfData = encodedData;
    clock_t decompressed = clock();

    while(cellIdx < area) {
        char header = *encodedData;
        encodedData++;
        size_t readData = tsc_tsc_decodeChunk(grid, &cellIdx, encodedData, header);
        encodedData += readData;
    }

    clock_t decoded = clock();

    printf("Timings\n");
    printf("Decompress: %f\n", (float)(decompressed - start) / CLOCKS_PER_SEC);
    printf("Decode: %f\n", (float)(decoded - decompressed) / CLOCKS_PER_SEC);

    free(eBase64);
    RL_FREE(deflated);
    RL_FREE(startOfData);
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
    v3.flags = TSC_SAVING_COMPATIBILITY;
    tsc_saving_register(v3);

    tsc_saving_format v2 = {};
    v2.name = "V2";
    v2.header = "V2;";
    v2.decode = tsc_v2_decode;
    v2.encode = NULL;
    v2.flags = TSC_SAVING_COMPATIBILITY;
    tsc_saving_register(v2);

    tsc_saving_format v1 = {};
    v1.name = "V1";
    v1.header = "V1;";
    v1.decode = tsc_v1_decode;
    v1.encode = NULL;
    v1.flags = TSC_SAVING_COMPATIBILITY;
    tsc_saving_register(v1);
    
    tsc_saving_format tsc = {};
    tsc.name = "TSC";
    tsc.header = "TSC;";
    tsc.decode = tsc_tsc_decode;
    tsc.encode = tsc_tsc_encode;
    tsc.flags = 0;
    tsc_saving_register(tsc);
}

char *tsc_saving_safeFast(tsc_grid *grid) {
    tsc_buffer buffer = tsc_saving_newBufferCapacity(NULL, grid->width * grid->height);
    if(!tsc_tsc_encode(&buffer, grid)) {
        tsc_saving_deleteBuffer(buffer);
        return NULL;
    }
    return buffer.mem;
}
