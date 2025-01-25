#include "replicators.h"

void more_gens_doReplicator(tsc_cell *cell, int x, int y, int ux, int uy, void *payload) {
    int fx = tsc_grid_frontX(x, cell->rot);
    int fy = tsc_grid_frontY(y, cell->rot);
    tsc_cell *source = tsc_grid_get(currentGrid, fx, fy);
    if(source == NULL) return;
    if(!tsc_cell_canGenerate(currentGrid, source, fx, fy, cell, x, y, cell->rot)) return;
    tsc_grid_push(currentGrid, fx, fy, cell->rot, 1, source);
}
