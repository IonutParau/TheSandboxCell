#ifndef TSC_NUI
#define TSC_NUI

#include <stdbool.h>
#include <stddef.h>

typedef unsigned int tsc_nui_color;

typedef struct tsc_nui_geometry {
    float x;
    float y;
    float w;
    float h;
} tsc_nui_geometry;

typedef struct tsc_nui_button {
    float pressTime;
    float longPressTime;
    bool wasClicked;
    bool clicked;
    bool rightClick;
    bool hovered;
} tsc_nui_buttonState;

tsc_nui_buttonState *tsc_nui_newButton();

#define TSC_NUI_BUTTON_CLICK 1
#define TSC_NUI_BUTTON_PRESS 2
#define TSC_NUI_BUTTON_LONGPRESS 3
#define TSC_NUI_BUTTON_RIGHTCLICK 4
#define TSC_NUI_BUTTON_RIGHTPRESS 5
#define TSC_NUI_BUTTON_RIGHTLONGPRESS 6
#define TSC_NUI_BUTTON_HOVER 7

bool tsc_nui_checkButton(tsc_nui_buttonState *button, int action);

#define TSC_NUI_INPUT_DEFAULT 0
#define TSC_NUI_INPUT_SIZE 1
#define TSC_NUI_INPUT_FLOAT 2
#define TSC_NUI_INPUT_SIGNED 4

typedef struct tsc_nui_input {
    char *text;
    size_t textlen;
    const char *placeholder;
    size_t flags;
    bool submitted;
    bool focused;
} tsc_nui_input;

tsc_nui_input *tsc_nui_newInput(const char *placeholder, size_t flags);

typedef struct tsc_nui_theme {
    tsc_nui_color color;
    unsigned int fontSize;
    unsigned int spacing;
    unsigned int borderSize;
    unsigned int cursorSize;
} tsc_nui_theme;

#define TSC_NUI_TEXT 0
#define TSC_NUI_STACK 1
#define TSC_NUI_ROW 2
#define TSC_NUI_COLUMN 3
#define TSC_NUI_TRANSLATE 4
#define TSC_NUI_BUTTON 5

typedef struct tsc_nui_element {
    char kind;
    tsc_nui_theme themeData;
    union {
        struct tsc_nui_element *child;
        struct tsc_nui_frame *children;
    };
    union {
        const char *text;
        tsc_nui_buttonState *button;
        struct {
            float x;
            float y;
        } translate;
    };
} tsc_nui_element;

#define TSC_NUI_MAXELEMENTS 64

typedef struct tsc_nui_frame {
    size_t len;
    tsc_nui_element *elements[TSC_NUI_MAXELEMENTS];
} tsc_nui_frame;

tsc_nui_frame *tsc_nui_newFrame();
void tsc_nui_freeFrame(tsc_nui_frame *frame);
tsc_nui_frame *tsc_nui_newTmpFrame();

tsc_nui_element *tsc_nui_newElement(char kind);
void tsc_nui_pushElement(tsc_nui_element *element);
tsc_nui_element *tsc_nui_popElement();

#define TSC_NUI_MAXFRAMES 64

double tsc_nui_scaling();

tsc_nui_theme tsc_nui_currentTheme(tsc_nui_theme *theme);

void tsc_nui_setColor(unsigned int color);
unsigned int tsc_nui_getColor();

void tsc_nui_setFontSize(unsigned int size);
unsigned int tsc_nui_getFontSize();

void tsc_nui_setSpacing(unsigned int spacing);
unsigned int tsc_nui_getSpacing();

void tsc_nui_setBorderSize(unsigned int size);
unsigned int tsc_nui_getBorderSize();

void tsc_nui_setCursorSize(unsigned int size);
unsigned int tsc_nui_getCursorSize();

void tsc_nui_pushFrame(tsc_nui_frame *frame);
tsc_nui_frame *tsc_nui_popFrame();

void tsc_nui_beginStack();
void tsc_nui_endStack();

void tsc_nui_beginRow();
void tsc_nui_endRow();

void tsc_nui_beginColumn();
void tsc_nui_endColumn();

void tsc_nui_text(const char *text);

void tsc_nui_translate(float x, float y);
// returns true if hovered
bool tsc_nui_button(tsc_nui_buttonState *button);

tsc_nui_geometry tsc_nui_position(tsc_nui_element *element, tsc_nui_geometry parent);

void tsc_nui_renderElement(tsc_nui_element *element, tsc_nui_geometry parent);
void tsc_nui_updateElement(tsc_nui_element *element, tsc_nui_geometry parent);
bool tsc_nui_absorbsElement(tsc_nui_element *element, tsc_nui_geometry parent, float x, float y);

void tsc_nui_render();
void tsc_nui_update();
bool tsc_nui_absorbs(float x, float y);

#endif
