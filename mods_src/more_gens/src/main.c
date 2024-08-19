#include "libtsc.h"
#include "replicators.h"
#include "common.h"
#include <assert.h>

more_gens_ids_t more_gens_ids;

void more_gens_init() {
    more_gens_ids.replicator = tsc_registerCell("replicator", "Replicator", "Generates the cell in front");
    tsc_textures_load(defaultResourcePack, "replicator", "replicator.png");

    tsc_celltable *reptable = tsc_cell_newTable(more_gens_ids.replicator);
    reptable->update = more_gens_doReplicator;

    tsc_category *gens = tsc_getCategory(tsc_rootCategory(), "Generators");
    assert(gens != NULL);

    tsc_subtick_t *subtick = tsc_subtick_find(tsc_strintern("generators"));
    tsc_subtick_addCell(subtick, more_gens_ids.replicator);

    tsc_addCell(gens, more_gens_ids.replicator);
}
