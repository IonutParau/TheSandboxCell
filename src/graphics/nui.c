#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "nui.h"
#include "../utils.h"
#include "resources.h"

tsc_nui_buttonState *tsc_nui_newButton() {
    tsc_nui_buttonState *button = malloc(sizeof(tsc_nui_buttonState));
    button->pressTime = 0;
    button->longPressTime = 0.5;
    button->clicked = false;
    button->wasClicked = false;
    button->hovered = false;
    button->rightClick = false;
    return button;
}

bool tsc_nui_checkButton(tsc_nui_buttonState *button, int action) {
    if(action == TSC_NUI_BUTTON_HOVER) return button->hovered;
    if(action == TSC_NUI_BUTTON_CLICK) return button->clicked && !button->rightClick;
    if(action == TSC_NUI_BUTTON_RIGHTCLICK) return button->clicked && button->rightClick;
    if(action == TSC_NUI_BUTTON_PRESS) return button->clicked && button->pressTime < button->longPressTime && !button->rightClick;
    if(action == TSC_NUI_BUTTON_LONGPRESS) return button->clicked && button->pressTime >= button->longPressTime && !button->rightClick;
    if(action == TSC_NUI_BUTTON_RIGHTPRESS) return button->clicked && button->pressTime < button->longPressTime && button->rightClick;
    if(action == TSC_NUI_BUTTON_RIGHTLONGPRESS) return button->clicked && button->pressTime >= button->longPressTime && button->rightClick;
    return false;
}

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

tsc_nui_element *tsc_nui_newElement(char kind) {
    tsc_nui_element *element = tsc_talloc(sizeof(tsc_nui_element));
    element->kind = kind;
    element->themeData = tsc_nui_globalTheme;
    return element;
}

static tsc_nui_frame *tsc_nui_topFrame() {
    return tsc_nui_frames[tsc_nui_frameLen - 1];
}

void tsc_nui_pushElement(tsc_nui_element *element) {
    tsc_nui_frame *frame = tsc_nui_topFrame();
    frame->elements[frame->len++] = element;
}

tsc_nui_element *tsc_nui_popElement() {
    tsc_nui_frame *frame = tsc_nui_topFrame();
    return frame->elements[--frame->len];
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
    frame->len = 0;
}

void tsc_nui_bringBackFrame(tsc_nui_frame *frame) {
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
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_STACK);
    element->children = frame;
    tsc_nui_pushElement(element);
}

void tsc_nui_beginRow() {
    tsc_nui_pushFrame(tsc_nui_newTmpFrame());
}

void tsc_nui_endRow() {
    tsc_nui_frame *frame = tsc_nui_popFrame();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_ROW);
    element->children = frame;
    tsc_nui_pushElement(element);
}

void tsc_nui_beginColumn() {
    tsc_nui_pushFrame(tsc_nui_newTmpFrame());
}

void tsc_nui_endColumn() {
    tsc_nui_frame *frame = tsc_nui_popFrame();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_COLUMN);
    element->children = frame;
    tsc_nui_pushElement(element);
}

void tsc_nui_text(const char *text) {
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_TEXT);
    element->text = text;
    tsc_nui_pushElement(element);
}

void tsc_nui_translate(float x, float y) {
    tsc_nui_element *child = tsc_nui_popElement();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_TRANSLATE);
    element->child = child;
    element->translate.x = x;
    element->translate.y = y;
    tsc_nui_pushElement(element);
}

bool tsc_nui_button(tsc_nui_buttonState *button) {
    tsc_nui_element *child = tsc_nui_popElement();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_BUTTON);
    element->child = child;
    element->button = button;
    tsc_nui_pushElement(element);
    return tsc_nui_checkButton(button, TSC_NUI_BUTTON_HOVER);
}

void tsc_nui_aligned(float x, float y) {
    tsc_nui_element *child = tsc_nui_popElement();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_ALIGN);
    element->child = child;
    element->alignment.x = x;
    element->alignment.y = y;
    tsc_nui_pushElement(element);
}

void tsc_nui_sized(float w, float h) {
    tsc_nui_element *child = tsc_nui_popElement();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_SIZED);
    element->child = child;
    element->size.x = w;
    element->size.y = h;
    tsc_nui_pushElement(element);
}

void tsc_nui_pad(float x, float y) {
    tsc_nui_element *child = tsc_nui_popElement();
    tsc_nui_element *element = tsc_nui_newElement(TSC_NUI_PAD);
    element->child = child;
    element->pad.x = x;
    element->pad.y = y;
    tsc_nui_pushElement(element);
}

static tsc_nui_geometry tsc_nui_rootGeometry() {
    double scale = tsc_nui_scaling();
    return (tsc_nui_geometry) {0, 0, GetScreenWidth() / scale, GetScreenHeight() / scale};
}

static tsc_nui_geometry tsc_nui_frameGeometry(tsc_nui_frame *frame, tsc_nui_geometry position, bool x, bool y) {
    float w = 0;
    float h = 0;

    for(size_t i = 0; i < frame->len; i++) {
        tsc_nui_element *element = frame->elements[i];
        tsc_nui_geometry geo = tsc_nui_position(element, position);

        if(x) w += geo.w;
        else if(w < geo.w) w = geo.w;
        if(y) h += geo.h;
        else if(h < geo.h) h = geo.h;
    }

    return (tsc_nui_geometry) {0, 0, w, h};
}

static void tsc_nui_drawFrame(tsc_nui_frame *frame, tsc_nui_geometry position, bool x, bool y) {
    for(size_t i = 0; i < frame->len; i++) {
        tsc_nui_element *element = frame->elements[i];
        tsc_nui_renderElement(element, position);
        tsc_nui_geometry geo = tsc_nui_position(element, position);

        if(x) position.x += geo.w;
        if(y) position.y += geo.h;
    }
}

static void tsc_nui_updateFrame(tsc_nui_frame *frame, tsc_nui_geometry position, bool x, bool y) {
    for(size_t i = 0; i < frame->len; i++) {
        tsc_nui_element *element = frame->elements[i];
        tsc_nui_updateElement(element, position);
        tsc_nui_geometry geo = tsc_nui_position(element, position);

        if(x) position.x += geo.w;
        if(y) position.y += geo.h;
    }
}

static bool tsc_nui_absorbsFrame(tsc_nui_frame *frame, tsc_nui_geometry position, float mx, float my, bool x, bool y) {
    for(size_t i = 0; i < frame->len; i++) {
        tsc_nui_element *element = frame->elements[i];
        tsc_nui_geometry geo = tsc_nui_position(element, position);
        if(tsc_nui_absorbsElement(element, geo, mx, my)) {
            return true;
        }

        if(x) position.x += geo.w;
        if(y) position.y += geo.h;
    }
    return false;
}

tsc_nui_geometry tsc_nui_position(tsc_nui_element *element, tsc_nui_geometry parent) {
    tsc_nui_geometry geometry = parent;
    double scaling = tsc_nui_scaling();
    if(element->kind == TSC_NUI_TEXT) {
        Font font = font_get();
        // geometry is scaled so we dont scale it here
        double size = element->themeData.fontSize;
        double spacing = element->themeData.spacing;
        Vector2 v = MeasureTextEx(font, element->text, size, spacing);
        geometry.w = v.x;
        geometry.h = v.y;
    } else if(element->kind == TSC_NUI_TRANSLATE) {
        geometry.x += element->translate.x;
        geometry.y += element->translate.y;
    } else if(element->kind == TSC_NUI_STACK) {
        return tsc_nui_frameGeometry(element->children, parent, false, false);
    } else if(element->kind == TSC_NUI_ROW) {
        return tsc_nui_frameGeometry(element->children, parent, true, false);
    } else if(element->kind == TSC_NUI_COLUMN) {
        return tsc_nui_frameGeometry(element->children, parent, false, true);
    } else if(element->kind == TSC_NUI_BUTTON) {
        return tsc_nui_position(element->child, parent);
    } else if(element->kind == TSC_NUI_SIZED) {
        geometry.w = element->size.x;
        geometry.h = element->size.y;
    } else if(element->kind == TSC_NUI_PAD) {
        geometry = tsc_nui_position(element->child, parent);
        geometry.x += element->pad.x;
        geometry.y += element->pad.y;
        geometry.w += element->pad.x * 2;
        geometry.h += element->pad.y * 2;
    }
    return geometry;
}
        
static tsc_nui_geometry tsc_nui_alignedPosition(tsc_nui_geometry parent, tsc_nui_geometry child, float x, float y) {
    float offX = parent.w * x - child.w * x;
    float offY = parent.h * y - child.h * y;

    child.x += offX;
    child.y += offY;
    return child;
}

void tsc_nui_renderElement(tsc_nui_element *element, tsc_nui_geometry parent) {
start:;
    double scaling = tsc_nui_scaling();
    if(element->kind == TSC_NUI_TEXT) {
        Font font = font_get();
        double size = element->themeData.fontSize * scaling;
        double spacing = element->themeData.spacing * scaling;
        Color color = GetColor(element->themeData.color);
        Vector2 pos = {parent.x * scaling, parent.y * scaling};
        DrawTextEx(font, element->text, pos, size, spacing, color);
    } else if(element->kind == TSC_NUI_TRANSLATE) {
        parent.x += element->translate.x;
        parent.y += element->translate.y;
        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_BUTTON) {
        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_SIZED) {
        parent = tsc_nui_position(element, parent);
        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_ALIGN) {
        float x = element->alignment.x;
        float y = element->alignment.y;
        element = element->child;
        tsc_nui_geometry older = parent;
        parent = tsc_nui_position(element, parent);
        parent = tsc_nui_alignedPosition(older, parent, x, y);
        goto start;
    } else if(element->kind == TSC_NUI_PAD) {
        parent = tsc_nui_position(element, parent);
        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_COLUMN) {
        tsc_nui_drawFrame(element->children, parent, false, true);
    }
}

void tsc_nui_updateElement(tsc_nui_element *element, tsc_nui_geometry parent) {
start:;
    double scaling = tsc_nui_scaling();
    if(element->kind == TSC_NUI_TRANSLATE) {
        parent.x += element->translate.x;
        parent.y += element->translate.y;
        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_BUTTON) {
        tsc_nui_geometry geo = tsc_nui_position(element->child, parent);
        tsc_nui_buttonState *state = element->button;
        float mx = GetMouseX() / scaling;
        float my = GetMouseY() / scaling;
        if(mx >= geo.x && mx <= geo.x + geo.w && my >= geo.y && my <= geo.y + geo.h) state->hovered = true;
        else state->hovered = false;

        if(state->hovered) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state->clicked = true;
                state->rightClick = false;
            } else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                state->clicked = true;
                state->rightClick = true;
            } else state->clicked = false;
        } else state->clicked = false;
        if(state->clicked) {
            if(!state->wasClicked) state->pressTime = 0;
            state->pressTime += GetFrameTime();
        }

        state->wasClicked = state->clicked;

        element = element->child;
        goto start;
    } else if(element->kind == TSC_NUI_COLUMN) {
        tsc_nui_updateFrame(element->children, parent, false, true);
    }
}

bool tsc_nui_absorbsElement(tsc_nui_element *element, tsc_nui_geometry parent, float x, float y) {
    double scaling = tsc_nui_scaling();
    if(element->kind == TSC_NUI_TRANSLATE) {
        parent.x += element->translate.x;
        parent.y += element->translate.y;
        return tsc_nui_absorbsElement(element->child, parent, x, y);
    }
    return false;
}

void tsc_nui_render() {
    tsc_nui_drawFrame(tsc_nui_topFrame(), tsc_nui_rootGeometry(), false, false);
}

void tsc_nui_update() {
    tsc_nui_updateFrame(tsc_nui_topFrame(), tsc_nui_rootGeometry(), false, false);
}

bool tsc_nui_absorbs(float x, float y) {
    double scaling = tsc_nui_scaling();
    x /= scaling;
    y /= scaling;
    return tsc_nui_absorbsFrame(tsc_nui_topFrame(), tsc_nui_rootGeometry(), x, y, false, false);
}
