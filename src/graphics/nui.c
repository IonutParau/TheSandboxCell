#include <stdlib.h>
#include <raylib.h>
#include "nui.h"
#include "../utils.h"

size_t tsc_nui_frameLen = 0;
tsc_nui_frame *tsc_nui_frames[TSC_NUI_MAXFRAMES];
tsc_nui_theme tsc_nui_globalTheme = {
    .color = 0xFFFFFFFF,
    .borderSize = 20,
    .cursorSize = 10,
    .fontSize = 10,
    .spacing = 5,
};

tsc_nui_frame *tsc_nui_newFrame() {
    tsc_nui_frame *frame = malloc(sizeof(tsc_nui_frame));
    frame->len = 0;
    return frame;
}

void tsc_nui_freeFrame(tsc_nui_frame *frame) {
    free(frame);
}

tsc_nui_frame *tsc_nui_newTmpFrame() {
    tsc_nui_frame *frame = tsc_talloc(sizeof(tsc_nui_frame));
    frame->len = 0;
    return frame;
}

tsc_nui_element tsc_nui_newElement(char kind) {
    tsc_nui_element element;
    element.kind = kind;
    element.themeData = tsc_nui_globalTheme;
    return element;
}

void tsc_nui_pushElement(tsc_nui_element element) {
    tsc_nui_frame *frame = tsc_nui_frames[tsc_nui_frameLen - 1];
    frame->elements[frame->len++] = element;
}

double tsc_nui_scaling() {
    return (float)GetScreenHeight() / 1080;
}

tsc_nui_theme tsc_nui_currentTheme(tsc_nui_theme *theme) {
    if(theme != NULL) tsc_nui_globalTheme = *theme;
    return tsc_nui_globalTheme;
}

void tsc_nui_setColor(unsigned int color) {
    tsc_nui_globalTheme.color = color;
}

unsigned int tsc_nui_getColor() {
    return tsc_nui_globalTheme.color;
}

void tsc_nui_setFontSize(unsigned int size) {
    tsc_nui_globalTheme.fontSize = size;
}

unsigned int tsc_nui_getFontSize() {
    return tsc_nui_globalTheme.fontSize;
}

void tsc_nui_setSpacing(unsigned int spacing) {
    tsc_nui_globalTheme.spacing = spacing;
}

unsigned int tsc_nui_getSpacing() {
    return tsc_nui_globalTheme.spacing;
}

void tsc_nui_setBorderSize(unsigned int size) {
    tsc_nui_globalTheme.borderSize = size;
}

unsigned int tsc_nui_getBorderSize() {
    return tsc_nui_globalTheme.borderSize;
}

void tsc_nui_setCursorSize(unsigned int size) {
    tsc_nui_globalTheme.cursorSize = size;
}

unsigned int tsc_nui_getCursorSize() {
    return tsc_nui_globalTheme.cursorSize;
}

void tsc_nui_pushFrame(tsc_nui_frame *frame) {
    tsc_nui_frames[tsc_nui_frameLen++] = frame;
}

tsc_nui_frame *tsc_nui_popFrame() {
    return tsc_nui_frames[--tsc_nui_frameLen];
}

void tsc_nui_beginStack() {
    tsc_nui_pushFrame(tsc_nui_newTmpFrame());
}

void tsc_nui_endStack() {
    tsc_nui_frame *frame = tsc_nui_popFrame();
    tsc_nui_element element = tsc_nui_newElement(TSC_NUI_STACK);
    element.children = frame;
}

void tsc_nui_beginRow() {
    tsc_nui_pushFrame(tsc_nui_newTmpFrame());
}

void tsc_nui_endRow() {
    tsc_nui_frame *frame = tsc_nui_popFrame();
    tsc_nui_element element = tsc_nui_newElement(TSC_NUI_ROW);
    element.children = frame;
}

void tsc_nui_beginColumn() {
    tsc_nui_pushFrame(tsc_nui_newTmpFrame());
}

void tsc_nui_endColumn() {
    tsc_nui_frame *frame = tsc_nui_popFrame();
    tsc_nui_element element = tsc_nui_newElement(TSC_NUI_COLUMN);
    element.children = frame;
}

tsc_nui_geometry tsc_nui_position(tsc_nui_element *element, tsc_nui_geometry parent) {
    tsc_nui_geometry geometry = parent;
    return geometry;
}

void tsc_nui_render();
void tsc_nui_update();
void tsc_nui_absorbs(float x, float y);
