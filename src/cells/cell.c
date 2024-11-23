#include "grid.h"
#include "../utils.h"
#include "../graphics/resources.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

tsc_cell_id_pool_t builtin;

void tsc_init_builtin_ids() {
    builtin.push = tsc_strintern("push");
    builtin.slide = tsc_strintern("slide");
    builtin.mover = tsc_strintern("mover");
    builtin.trash = tsc_strintern("trash");
    builtin.enemy = tsc_strintern("enemy");
    builtin.generator = tsc_strintern("generator");
    builtin.placeable = tsc_strintern("place");
    builtin.rotator_cw = tsc_strintern("rotator_cw");
    builtin.rotator_ccw = tsc_strintern("rotator_ccw");
    builtin.empty = tsc_strintern("empty");
    builtin.wall = tsc_strintern("wall");

    tsc_celltable *placeTable = tsc_cell_newTable(builtin.placeable);
    placeTable->flags = TSC_FLAGS_PLACEABLE;

    builtin.textures.icon = tsc_strintern("icon");
    builtin.textures.copy = tsc_strintern("copy");
    builtin.textures.cut = tsc_strintern("cut");
    builtin.textures.del = tsc_strintern("delete");
    builtin.textures.setinitial = tsc_strintern("setinitial");
    builtin.textures.restoreinitial = tsc_strintern("restoreinitial");

    builtin.audio.destroy = tsc_strintern("destroy");
    builtin.audio.explosion = tsc_strintern("explosion");

    builtin.optimizations.gens[0] = tsc_allocOptimization("gen0");
    builtin.optimizations.gens[1] = tsc_allocOptimization("gen1");
    builtin.optimizations.gens[2] = tsc_allocOptimization("gen2");
    builtin.optimizations.gens[3] = tsc_allocOptimization("gen3");
}

tsc_cellArray_t tsc_cellArray_create(size_t len) {
    tsc_cellArray_t arr;
    arr.ids = malloc(sizeof(const char *) * len);
    arr.rots = malloc(sizeof(char) * len);
    arr.textures = malloc(sizeof(const char *) * len);
    arr.datas = malloc(sizeof(tsc_cellreg *) * len);
    arr.flags = malloc(sizeof(size_t) * len);
    arr.privates = malloc(sizeof(struct tsc_private_cell_stuff_do_not_touch) * len);
    arr.celltables = malloc(sizeof(struct tsc_celltable *) * len);
    for(size_t i = 0; i < len; i++) {
        arr.ids[i] = builtin.empty;
        arr.rots[i] = 0;
        arr.textures[i] = NULL;
        arr.datas[i] = NULL;
        arr.flags[i] = 0;
        arr.celltables[i] = NULL;
    }
    return arr;
}

void tsc_cellArray_realloc(tsc_cellArray_t *arr, size_t oldLen, size_t len) {
    // Fuck this for now // no, fuck you for now.

    arr->ids = realloc(arr->ids, len * sizeof(const char *));
    arr->rots = realloc(arr->rots, len * sizeof(char));
    arr->textures = realloc(arr->textures, len * sizeof(const char *));
    arr->datas = realloc(arr->datas, len * sizeof(tsc_cellreg *));
    arr->flags = realloc(arr->flags, len * sizeof(size_t));
    arr->privates = realloc(arr->privates, len * sizeof(struct tsc_private_cell_stuff_do_not_touch));
    arr->celltables = realloc(arr->celltables, len * sizeof(struct tsc_celltable *));

    for (size_t i = oldLen; i < len; i++) { // initialize new memory
    	arr->ids[i] = builtin.empty;
    	arr->rots[i] = 0;
		arr->textures[i] = NULL;
		arr->datas[i] = NULL;
		arr->flags[i] = 0;
		arr->celltables[i] = NULL;
    }
}

void tsc_cellArray_clear(tsc_cellArray_t *arr, size_t len) {
    for(size_t i = 0; i < len; i++) {
        tsc_cellreg *data = arr->datas[i];
        if(data != NULL) {
	        for(size_t i = 0; i < data->len; i++) {
	            free(data->values[i]);
	        }
	        free(data->keys);
	        free(data->values);
	        free(data);
			arr->datas[i] = NULL;
        }
        arr->ids[i] = builtin.empty;
        arr->rots[i] = 0;
        arr->flags[0] = 0;
        arr->celltables[i] = NULL;
        arr->textures[i] = NULL;
    }
}

void tsc_cellArray_clone(tsc_cellArray_t *arr, tsc_cellArray_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
    	arr->ids[i] = src->ids[i];
    	arr->rots[i] = src->rots[i];
     	arr->textures[i] = src->textures[i];
      	arr->flags[i] = src->flags[i];
       	arr->privates[i] = src->privates[i];
       	arr->celltables[i] = src->celltables[i];

        // now, the fucking hellish annoying piece of shit datas
        tsc_cellreg *data = src->datas[i];
        if(data != NULL) {
	        if (arr->datas[i] == NULL) {
	        	arr->datas[i] = malloc(sizeof(tsc_cellreg));
	        }
	        arr->datas[i]->len = data->len;
	        arr->datas[i]->keys = malloc(sizeof(char *) * data->len);
	        arr->datas[i]->values = malloc(sizeof(char *) * data->len);

	        for (size_t j = 0; j < data->len; j++) {
	        	arr->datas[i]->keys[i] = data->keys[i];
	         	arr->datas[i]->values[i] = tsc_strdup(data->values[i]);
	        }
        } else {
        	arr->datas[i] = NULL;
        }
    }
}

void tsc_cellArray_set(tsc_cellArray_t *arr, size_t i, const tsc_cell *cell) {
	tsc_cellreg *data = arr->datas[i];
	if(data != NULL) {
		for(size_t i = 0; i < data->len; i++) {
            free(data->values[i]);
        }
        free(data->keys);
        free(data->values);
        free(data);
        arr->datas[i] = NULL;
	}

	arr->ids[i] = tsc_cell_getID(cell);
	arr->rots[i] = tsc_cell_getRotation(cell);
	arr->flags[i] = tsc_cell_getFlags(cell);
	arr->textures[i] = tsc_cell_getTexture(cell);
	tsc_private_cell_stuff_do_not_touch *privates = tsc_cell_privateStuff(cell);
	if(privates != NULL) {
		arr->privates[i] = *privates;
	}
	tsc_celltable **ptable = tsc_cell_getCellTable(cell);
	if(ptable == NULL) {
		arr->celltables[i] = NULL;
	} else {
		arr->celltables[i] = *ptable; // persist the cache for performance
	}

	tsc_cellreg **cdata = tsc_cell_getCellRegistry(cell);
	if(cdata == NULL) return;
	if(*cdata == NULL) return;
	data = *cdata;
	arr->datas[i] = malloc(sizeof(tsc_cellreg));
	arr->datas[i]->len = data->len;
	arr->datas[i]->keys = malloc(sizeof(const char *) * data->len);
	arr->datas[i]->values = malloc(sizeof(const char *) * data->len);
	for(size_t i = 0; i < data->len; i++) {
		arr->datas[i]->keys[i] = data->keys[i];
		arr->datas[i]->values[i] = tsc_strdup(data->values[i]);
	}
}

void tsc_cellArray_destroy(tsc_cellArray_t arr, size_t len) {
    for(size_t i = 0; i < len; i++) {
        tsc_cellreg *data = arr.datas[i];
        for(size_t i = 0; i < data->len; i++) {
            free(data->values[i]);
        }
        free(data->keys);
        free(data->values);
        free(data);
    }
    free(arr.ids);
    free(arr.rots);
    free(arr.flags);
    free(arr.textures);
    free(arr.datas);
    free(arr.celltables);
    free(arr.privates);
}

tsc_cell *__attribute__((hot)) tsc_cell_create(const char *id, char rot) {
    tsc_cell *cell = malloc(sizeof(tsc_cell));
    cell->id = id;
    cell->texture = NULL;
    cell->rot = rot % 4;
    cell->data = NULL;
    cell->updated = false;
    cell->celltable = NULL;
    cell->flags = 0;
    // -1 means no interpolation please
    cell->lx = -1;
    cell->ly = -1;
    cell->addedRot = 0;
    return cell;
}

tsc_cell *__attribute__((hot)) tsc_cell_createReadonly(const char *id, char rot) {
	size_t addr = (size_t)id;
	addr += rot;
	addr |= tsc_cell_typeMask(TSC_CELL_LITE);
	return (tsc_cell *)addr;
}

tsc_cell *__attribute__((hot)) tsc_cell_clone(tsc_cell *cell) {
    // Cloning a lite cell yields back a lite cell.
    if(tsc_cell_checkType(cell, TSC_CELL_LITE)) {
        return cell;
    }
    tsc_cell *copy = malloc(sizeof(tsc_cell));
    copy->flags = tsc_cell_getFlags(cell);
    copy->texture = tsc_cell_getTexture(cell);
    copy->id = tsc_cell_getID(cell);
    copy->rot = tsc_cell_getRotation(cell);
    tsc_private_cell_stuff_do_not_touch privates = *tsc_cell_privateStuff(cell);
    copy->lx = privates.lx;
    copy->ly = privates.ly;
    copy->updated = privates.updated;
    copy->addedRot = privates.addedRot;
    if((*tsc_cell_getCellRegistry(cell)) == NULL) {
        copy->data = NULL;
        return copy;
    }
    copy->data = malloc(sizeof(tsc_cellreg));
    copy->data->len = cell->data->len;
    copy->data->keys = malloc(sizeof(const char *) * copy->data->len);
    memcpy(copy->data->keys, cell->data->keys, sizeof(const char *) * copy->data->len);
    copy->data->values = malloc(sizeof(const char *) * copy->data->len);
    for(size_t i = 0; i < cell->data->len; i++) {
        copy->data->values[i] = tsc_strdup(cell->data->values[i]);
    }
    return copy;
}

void __attribute__((hot)) tsc_cell_destroy(tsc_cell *cell) {
    if(tsc_cell_checkType(cell, TSC_CELL_LITE)) {
        return; // Don't care
    }
    tsc_cellreg **reg = tsc_cell_getCellRegistry(cell);
    if(*reg != NULL) {
        tsc_cellreg *data = *reg;
        if(data != NULL) {
        	for(size_t i = 0; i < data->len; i++) {
            	free(data->values[i]);
        	}
        	free(data->keys);
        	free(data->values);
        	free(data);
        }
    }
    if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
        free(cell);
        return;
    }
    *reg = NULL;
    tsc_cell_setID(cell, builtin.empty);
    tsc_cell_setRotation(cell, 0);
    tsc_cell_setFlags(cell, 0);
    tsc_cell_setTexture(cell, NULL);
    // Privates can stay I don't care
}

static const char *tsc_cell_getIDFromPtr(const char *id) {
	size_t addr = (size_t)id;
	addr &= ~3UL;
	return (const char *)addr;
}

static char tsc_cell_getRotFromPtr(const char *id) {
	size_t addr = (size_t)id;
	addr &= 3UL;
	return addr;
}

const char *tsc_cell_getID(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->cells.ids[i];
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		return cell->id;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->bgs.ids[i];
	}
	return tsc_cell_getIDFromPtr((const char *)tsc_cell_stripType(cell));
}

void tsc_cell_setID(tsc_cell *cell, const char *id) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->cells.ids[i] = id;
		currentGrid->cells.celltables[i] = NULL;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		cell->id = id;
		cell->celltable = NULL;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->bgs.ids[i] = id;
		currentGrid->bgs.celltables[i] = NULL;
		return;
	}
}

char tsc_cell_getRotation(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->cells.rots[i];
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		return cell->rot;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->bgs.rots[i];
	}
	return tsc_cell_getRotFromPtr((const char *)tsc_cell_stripType(cell));
}

void tsc_cell_setRotation(tsc_cell *cell, char rot) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->cells.rots[i] = rot;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		cell->rot = rot;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->bgs.rots[i] = rot;
		return;
	}
}

void tsc_cell_rotate(tsc_cell *cell, signed char amount) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		char rot = currentGrid->cells.rots[i];
		currentGrid->cells.privates[i].addedRot += amount;
		while(amount < 0) amount += 4;
		rot += amount;
		rot %= 4;
		currentGrid->cells.rots[i] = rot;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		cell->rot += amount;
		while(cell->rot < 0) cell->rot += 4;
		cell->rot %= 4;
		cell->addedRot += amount;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		char rot = currentGrid->bgs.rots[i];
		currentGrid->bgs.privates[i].addedRot += amount;
		while(amount < 0) amount += 4;
		rot += amount;
		rot %= 4;
		currentGrid->bgs.rots[i] = rot;
		return;
	}
}

const char *tsc_cell_getTexture(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->cells.textures[i];
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		return cell->texture;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->bgs.textures[i];
	}
	return NULL;
}

void tsc_cell_setTexture(tsc_cell *cell, const char *id) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->cells.textures[i] = id;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		cell->texture = id;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->bgs.textures[i] = id;
		return;
	}
}

size_t tsc_cell_getFlags(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->cells.flags[i];
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		return cell->flags;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->bgs.flags[i];
	}
	return 0;
}

void tsc_cell_setFlags(tsc_cell *cell, size_t flags) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->cells.flags[i] = flags;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		cell->flags= flags;
		return;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		currentGrid->bgs.flags[i] = flags;
		return;
	}
}

const char *tsc_cell_get(const tsc_cell *cell, const char *key) {
    tsc_cellreg **reg = tsc_cell_getCellRegistry(cell);
    if(reg == NULL) return NULL;
    if(*reg == NULL) return NULL;
    tsc_cellreg *data = *reg;
    for(size_t i = 0; i < data->len; i++) {
        if(tsc_streql(data->keys[i], key)) {
            return data->values[i];
        }
    }
    return NULL;
}

const char *tsc_cell_nthKey(const tsc_cell *cell, size_t idx) {
    tsc_cellreg **reg = tsc_cell_getCellRegistry(cell);
    if(reg == NULL) return NULL;
    if(*reg == NULL) return NULL;
    tsc_cellreg *data = *reg;
    if(idx >= data->len) return NULL;
    return data->keys[idx];
}

void tsc_cell_set(tsc_cell *cell, const char *key, const char *value) {
    tsc_cellreg **reg = tsc_cell_getCellRegistry(cell);
    if(reg == NULL) return; // there's nothing we can do. Lite cells are read-only.
    tsc_cellreg *data = *reg;
    if(value == NULL) {
        if(data == NULL) return;
        // Remove
        size_t j = 0;
        for(size_t i = 0; i < data->len; i++) {
            if(tsc_streql(data->keys[i], key)) {
                free(data->values[i]);
                break;
            }
            data->keys[j] = data->keys[i];
            data->values[j] = data->values[i];
            j++;
        }
        if(j != data->len) {
            // Something was removed
            data->len = j;
            data->keys = realloc(data->keys, sizeof(const char *) * j);
            data->values = realloc(data->values, sizeof(const char *) * j);
        }
        return;
    }
    if(data == NULL) {
        *reg = data = malloc(sizeof(tsc_cellreg));
        data->len = 0;
        data->keys = NULL;
        data->values = NULL;
    }
    for(size_t i = 0; i < data->len; i++) {
        if(tsc_streql(data->keys[i], key)) {
            free(data->values[i]);
            data->values[i] = tsc_strdup(value);
            return;
        }
    }
    size_t idx = data->len++;
    data->keys = realloc(cell->data->keys, sizeof(const char *) * data->len);
    data->keys[idx] = tsc_strintern(key);
    data->values = realloc(cell->data->values, sizeof(const char *) * data->len);
    data->values[idx] = tsc_strdup(value);
}

static bool tsc_is_vanilla(const char *id) {
	const char *ids[] = {builtin.push, builtin.empty, builtin.mover,
		builtin.rotator_cw, builtin.rotator_ccw, builtin.generator,
		builtin.enemy, builtin.trash, builtin.placeable};
	int idc = sizeof(ids) / sizeof(const char *);
	for(size_t i = 0; i < idc; i++) {
		if(ids[i] == id) return true;
	}
	return false;
}

// Invalid for lite cells
void tsc_cell_swap(tsc_cell *a, tsc_cell *b) {
	const char *aID = tsc_cell_getID(a);
	const char *bID = tsc_cell_getID(b);
	if(tsc_is_vanilla(aID) && tsc_is_vanilla(bID)) {
		char aRot = tsc_cell_getRotation(a);
		tsc_private_cell_stuff_do_not_touch *apriv = tsc_cell_privateStuff(a);
		char bRot = tsc_cell_getRotation(b);
		tsc_private_cell_stuff_do_not_touch *bpriv = tsc_cell_privateStuff(b);

		tsc_cell_setID(a, bID);
		tsc_cell_setRotation(a, bRot);
		tsc_cell_setID(b, aID);
		tsc_cell_setRotation(b, aRot);

		if(apriv != NULL && bpriv != NULL) {
			tsc_private_cell_stuff_do_not_touch tmp = *apriv;
			*apriv = *bpriv;
			*bpriv = tmp;
		}
		return;
	}
    // Lite cells make this process garbage
    tsc_cell c = tsc_cell_dump(a);
    tsc_cell d = tsc_cell_dump(b);
    // This means we can't write b to a. So delete b.
    if(tsc_cell_checkType(a, TSC_CELL_LITE)) {
        // NASTY soundness issues here
        tsc_cell_destroy(b);
    } else {
        tsc_cell_enter(a, d);
    }
    // Can't write to a to b, so kill a.
    if(tsc_cell_checkType(b, TSC_CELL_LITE)) {
        tsc_cell_destroy(a);
    } else {
        tsc_cell_enter(b, c);
    }
}

typedef struct tsc_cell_table_arr {
    tsc_celltable **tables;
    const char **ids;
    size_t tablec;
} tsc_cell_table_arr;

static tsc_cell_table_arr cell_table_arr = {NULL, NULL, 0};

tsc_celltable *tsc_cell_newTable(const char *id) {
    tsc_celltable *table = malloc(sizeof(tsc_celltable));
    tsc_celltable empty = {NULL, NULL, NULL};
    *table = empty;
    size_t idx = cell_table_arr.tablec++;
    cell_table_arr.tables = realloc(cell_table_arr.tables, sizeof(tsc_celltable *) * cell_table_arr.tablec);
    cell_table_arr.ids = realloc(cell_table_arr.ids, sizeof(const char *) * cell_table_arr.tablec);
    cell_table_arr.tables[idx] = table;
    cell_table_arr.ids[idx] = id;
    return table;
}

// HIGH priority for optimization
// While this is "technically" not thread-safe, you can use it as if it is.
tsc_celltable *tsc_cell_getTable(tsc_cell *cell) {
    // Locally cached VTable (id should never be assigned to so this is fine)
    tsc_celltable **ptable = tsc_cell_getCellTable(cell);
    const char *id = tsc_cell_getID(cell);
    // LITE CELL OH NO
    if(ptable == NULL) {
        for(size_t i = 0; i < cell_table_arr.tablec; i++) {
            if(cell_table_arr.ids[i] == id) {
                tsc_celltable *table = cell_table_arr.tables[i];
                return table;
            }
        }
        return NULL;
    }
    tsc_celltable *table = *ptable;
    if(table != NULL) return table;

    for(size_t i = 0; i < cell_table_arr.tablec; i++) {
        if(cell_table_arr.ids[i] == id) {
            tsc_celltable *table = cell_table_arr.tables[i];
            *ptable = table;
            return table;
        }
    }

    // Table not found.
    return NULL;
}

size_t tsc_cell_getTableFlags(tsc_cell *cell) {
    tsc_celltable *table = tsc_cell_getTable(cell);
    if(table == NULL) return 0;
    return table->flags;
}

tsc_private_cell_stuff_do_not_touch *tsc_cell_privateStuff(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return &currentGrid->cells.privates[i];
	}
	if(tsc_cell_checkType(cell, TSC_CELL_LITE)) return NULL;
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return &currentGrid->bgs.privates[i];
	}
	return &((tsc_cell *)cell)->privates;
}

size_t tsc_cell_typeMask(char type) {
    return ((size_t)type) << (sizeof(size_t) * 8 - 2);
}

bool tsc_cell_checkType(const tsc_cell *cell, char type) {
    size_t addr = (size_t)cell;
    size_t mask = tsc_cell_typeMask(type);
    return (addr & tsc_cell_typeMask(3)) == mask;
}

size_t tsc_cell_stripType(const tsc_cell *cell) {
    size_t addr = (size_t)cell;
    return addr & ~tsc_cell_typeMask(3); // 3 is 0b11.
}

tsc_celltable **tsc_cell_getCellTable(const tsc_cell *cell) {
    if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
        // Die.
        return &((tsc_cell *)cell)->celltable;
    }
    if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
        size_t i = tsc_cell_stripType(cell);
        return currentGrid->cells.celltables + i;
    }
    if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
        size_t i = tsc_cell_stripType(cell);
        return currentGrid->bgs.celltables + i;
    }
    return NULL;
}

tsc_cellreg **tsc_cell_getCellRegistry(const tsc_cell *cell) {
	if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
		return &((tsc_cell *)cell)->data;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_LITE)) {
		return NULL;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_POSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->cells.datas + i;
	}
	if(tsc_cell_checkType(cell, TSC_CELL_BGPOSPTR)) {
		size_t i = tsc_cell_stripType(cell);
		return currentGrid->bgs.datas + i;
	}
	return NULL;
}

tsc_cell tsc_cell_dump(const tsc_cell *cell) {
    if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
        return *cell; // so difficult
    }
    tsc_cell out;
    out.id = tsc_cell_getID(cell);
    out.rot = tsc_cell_getRotation(cell);
    out.texture = tsc_cell_getTexture(cell);
    out.flags = tsc_cell_getFlags(cell);
    tsc_celltable **ptable = tsc_cell_getCellTable(cell);
    if(ptable != NULL) {
    	out.celltable = *ptable;
    } else {
    	out.celltable = NULL;
    }
    tsc_cellreg **preg = tsc_cell_getCellRegistry(cell);
    if(preg != NULL) {
    	out.data = *preg;
    } else {
    	out.data = NULL;
    }
    tsc_private_cell_stuff_do_not_touch *privates = tsc_cell_privateStuff(cell);
    if(privates != NULL) {
    	out.privates = *privates;
    }
    return out;
}

void tsc_cell_enter(tsc_cell *cell, tsc_cell dumped) {
    if(tsc_cell_checkType(cell, TSC_CELL_RAWPTR)) {
        *cell = dumped; // so hard
        return;
    }
    if(tsc_cell_checkType(cell, TSC_CELL_LITE)) {
    	return; // Kimpossible
    }
    tsc_cell_setID(cell, dumped.id);
    tsc_cell_setRotation(cell, dumped.rot);
    tsc_cell_setTexture(cell, dumped.texture);
    tsc_cell_setFlags(cell, dumped.flags);
    *tsc_cell_getCellTable(cell) = dumped.celltable;
    *tsc_cell_getCellRegistry(cell) = dumped.data;
    *tsc_cell_privateStuff(cell) = dumped.privates;
}

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    const char *id = tsc_cell_getID(cell);
    if(id == builtin.wall) return 0;
    if(id == builtin.slide) return  dir % 2 == tsc_cell_getRotation(cell) % 2;

    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 1;
    if(celltable->canMove == NULL) return 1;
    return celltable->canMove(grid, cell, x, y, dir, forceType, force, celltable->payload);
}

float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    if(tsc_cell_getID(cell) == builtin.mover && tsc_streql(forceType, "push")) {
        if(tsc_cell_getRotation(cell) == dir) return 1;
        if((tsc_cell_getRotation(cell) + 2) % 4 == dir) return -1;

        return 0;
    }

    return 0;
}

int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir) {
    // Can't generate air
    const char *id = tsc_cell_getID(cell);
    if(id == builtin.empty) return 0;
    return 1;
}

int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating) {
    const char *id = tsc_cell_getID(cell);
    if(id == builtin.trash) return 1;
    if(id == builtin.enemy) return 1;
    return 0;
}

void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating) {
    const char *id = tsc_cell_getID(cell);
    if(id == builtin.enemy) {
        tsc_cell *empty = tsc_cell_createReadonly(builtin.empty, 0);
        tsc_grid_set(grid, x, y, empty);
        tsc_sound_play(builtin.audio.explosion);
    }
    if(id == builtin.trash) {
        tsc_sound_play(builtin.audio.destroy);
    }
}

int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {
    return 0;
}

void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {

}

char *tsc_cell_signal(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy) {
    tsc_celltable *table = tsc_cell_getTable(cell);
    if(table == NULL) return NULL;
    if(table->signal == NULL) return NULL;
    return table->signal(cell, x, y, protocol, data, sender, sx, sy, table->payload);
}
