#ifndef TSC_UI_H
#define TSC_UI_H

#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>

#define WIDTH(x) (((double)(x) / 100.0) * GetScreenWidth())
#define HEIGHT(y) (((double)(y) / 100.0) * GetScreenHeight())

extern const char *tsc_currentMenu;

typedef struct ui_frame ui_frame;

#define UI_BUTTON_CLICK 1
#define UI_BUTTON_PRESS 2
#define UI_BUTTON_LONGPRESS 3
#define UI_BUTTON_RIGHTCLICK 4
#define UI_BUTTON_RIGHTPRESS 5
#define UI_BUTTON_RIGHTLONGPRESS 6
#define UI_BUTTON_HOVER 7

typedef struct ui_button {
    float pressTime;
    float longPressTime;
    Color color;
    int shrinking;
    bool wasClicked;
    bool clicked;
    bool rightClick;
    bool hovered;
} ui_button;

#define UI_INPUT_DEFAULT 0
// Ideal for sizes. Only allows 0123456789
#define UI_INPUT_SIZE 1
// Like SIZE, but it allows a single .
#define UI_INPUT_FLOAT 2
#define UI_INPUT_SIGNED 4

typedef struct ui_input {
    char *text;
    size_t textlen;
    const char *placeholder;
    size_t maxlen;
    size_t flags;
} ui_input;

typedef struct ui_slider {
    double value;
    double min;
    double max;
    size_t segments;
} ui_slider;

typedef struct ui_scrollable {
    RenderTexture texture;
    int width;
    int height;
    int offx;
    int offy;
} ui_scrollable;

ui_frame *tsc_ui_newFrame();
void tsc_ui_destroyFrame(ui_frame *frame);
void tsc_ui_pushFrame(ui_frame *frame);
// like tsc_ui_pushFrame, but it does not "clear" what was before.
// This is useful if building the UI is slow, as you can do so conditionally.
// TSC_UI is meant to be a mix of immediate-mode and traditional component tree UIs, and this function is that bridge.
void tsc_ui_bringBackFrame(ui_frame *frame);
void tsc_ui_reset();
void tsc_ui_popFrame();

// Do everything
void tsc_ui_update(double delta);
void tsc_ui_render();
int tsc_ui_absorbedPointer(int x, int y);

// Containers
void tsc_ui_pushRow();
void tsc_ui_finishRow();
void tsc_ui_pushColumn();
void tsc_ui_finishColumn();
void tsc_ui_pushStack();
void tsc_ui_finishStack();

// States
ui_button *tsc_ui_newButtonState();
ui_input *tsc_ui_newInputState(const char *placeholder, size_t maxlen, size_t flags);
ui_slider *tsc_ui_newSliderState(double min, double max);
ui_scrollable *tsc_ui_newScrollableState();

void tsc_ui_clearButtonState(ui_button *button);
void tsc_ui_clearInputState(ui_input *input);
void tsc_ui_clearSliderState(ui_slider *slider);
void tsc_ui_clearScrollableState(ui_scrollable *scrollable);

// Stateful components
int tsc_ui_button(ui_button *state);
int tsc_ui_checkbutton(ui_button *state);
const char *tsc_ui_input(ui_input *state, int width, int height);
double tsc_ui_slider(ui_slider *state, int width, int height, int thickness);

// Stateless components

void tsc_ui_image(const char *id, int width, int height);
void tsc_ui_text(const char *text, int size, Color color);
void tsc_ui_space(int amount);

// Stateless components with a child

void tsc_ui_scrollable(int width, int height);
void tsc_ui_translate(int x, int y);
void tsc_ui_pad(int x, int y);
void tsc_ui_align(float x, float y, int width, int height);
void tsc_ui_center(int width, int height);
void tsc_ui_box(Color background);
void tsc_ui_tooltip(const char *name, int nameSize, const char *description, int descSize, int maxLineLen);

// Macros
#define tsc_ui_row(body) do {tsc_ui_pushRow(); body; tsc_ui_finishRow();} while(0)
#define tsc_ui_column(body) do {tsc_ui_pushColumn(); body; tsc_ui_finishColumn();} while(0)
#define tsc_ui_stack(body) do {tsc_ui_pushStack(); body; tsc_ui_finishStack();} while(0)

#endif
