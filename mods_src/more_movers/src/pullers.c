#include "libtsc.h"
#include "pullers.h"
#include <stdio.h>

void moremovers_doPuller(tsc_cell *cell, int x, int y, int ux, int uy, void *payload) {
    tsc_grid_pull(currentGrid, x, y, cell->rot, 1, NULL);
}
