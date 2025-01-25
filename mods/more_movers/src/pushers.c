#include "libtsc.h"
#include "pushers.h"

void moremovers_doFan(tsc_cell *cell, int x, int y, int ux, int uy, void *payload) {
    int fx = tsc_grid_frontX(x, cell->rot);
    int fy = tsc_grid_frontY(y, cell->rot);
    tsc_grid_push(currentGrid, fx, fy, cell->rot, 1, NULL);
}
