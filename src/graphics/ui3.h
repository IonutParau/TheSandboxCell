#ifndef TSC_UI2
#define TSC_UI2

#include <stdbool.h>
#include <stddef.h>

typedef unsigned int tsc_color;

typedef struct tsc_context {
    float x;
    float y;
    float w;
    float h;
} tsc_context;

typedef struct tsc_button {
    float pressTime;
    float longPressTime;
    bool wasClicked;
    bool clicked;
    bool rightClick;
    bool hovered;
} tsc_button;

tsc_button *tsc_newButton();

#define TSC_BUTTON_CLICK 1
#define TSC_BUTTON_PRESS 2
#define TSC_BUTTON_LONGPRESS 3
#define TSC_BUTTON_RIGHTCLICK 4
#define TSC_BUTTON_RIGHTPRESS 5
#define TSC_BUTTON_RIGHTLONGPRESS 6
#define TSC_BUTTON_HOVER 7

bool tsc_checkButton(tsc_button *button, int action);

#define TSC_INPUT_DEFAULT 0
#define TSC_INPUT_SIZE 1
#define TSC_INPUT_FLOAT 2
#define TSC_INPUT_SIGNED 4

typedef struct tsc_input {
    char *text;
    size_t textlen;
    const char *placeholder;
    size_t flags;
    bool submitted;
    bool focused;
} tsc_input;

tsc_input *tsc_newInput(const char *placeholder, size_t flags);

typedef struct tsc_theme {
    unsigned int color;
    unsigned int fontSize;
    unsigned int spacing;
    unsigned int borderSize;
    unsigned int cursorSize;
} tsc_theme;

typedef struct tsc_element {
    char kind;
    tsc_theme themeData;
    union {
        struct tsc_element *child;
        struct tsc_frame *children;
    };
    union {
        tsc_button *button;
    };
} tsc_element;

typedef struct tsc_frame {
    tsc_element *elements;
    size_t len;
    size_t cap;
} tsc_frame;

#endif
