#ifndef TSC_GRAPHICS_RENDERING
#define TSC_GRAPHICS_RENDERING

#include "../cells/grid.h"

extern const char *currentId;
extern char currentRot;

void tsc_setupRendering();
void tsc_drawCell(tsc_cell *cell, int x, int y, double opacity, int gridRepeat);
int tsc_cellMouseX();
int tsc_cellMouseY();
void tsc_drawGrid();
void tsc_handleRenderInputs();

#endif
