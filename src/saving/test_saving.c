#include "saving.h"
#include "../testing.h"
#include "test_saving.h"
#include <stdbool.h>

void tsc_testSaving() {
    tsc_test("Encoding V3");
    tsc_grid *grid = tsc_createGrid("test", 100, 100, NULL, NULL);

    tsc_cell cell = tsc_cell_create(builtin.generator, 3);
    tsc_grid_set(grid, 50, 50, &cell);
    cell.rot = 0;
    tsc_grid_set(grid, 51, 50, &cell);
    cell.rot = 1;
    tsc_grid_set(grid, 51, 51, &cell);
    cell.rot = 2;
    tsc_grid_set(grid, 50, 51, &cell);
    cell.id = builtin.rotator_ccw;
    cell.rot = 0;
    tsc_grid_set(grid, 52, 52, &cell);

    tsc_grid *out = tsc_createGrid("out", 1, 1, NULL, NULL);
    tsc_saving_buffer buffer = tsc_saving_newBuffer(NULL);
    tsc_assert(tsc_saving_encodeWith(&buffer, grid, "V3") == true, "V3 encoding failed");
    tsc_saving_decodeWith(buffer.mem, out, "V3");
    tsc_saving_deleteBuffer(buffer);

    tsc_assert(grid->width == out->width, "width went from %d to %d", grid->width, out->width);
    tsc_assert(grid->height == out->height, "height went from %d to %d", grid->height, out->height);

    for(int x = 0; x < grid->width; x++) {
        for(int y = 0; y < grid->height; y++) {
            tsc_cell *a = tsc_grid_get(grid, x, y);
            tsc_cell *b = tsc_grid_get(out, x, y);

            // They should be the exact same pointer
            tsc_assert(a->id == b->id, "at %d,%d cell ID %s became %s", x, y, a->id, b->id);
            tsc_assert(a->rot == b->rot, "at %d,%d cell rot %d became %d", x, y, a->rot, b->rot);
        }
    }

    tsc_deleteGrid(grid);
}
