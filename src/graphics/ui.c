#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <assert.h>
#include "ui.h"
#include "../utils.h"
#include "resources.h"

// I pray, almost every day, that one day I can forget I wrote this

const char *tsc_currentMenu = "main";

typedef struct ui_node ui_node;

typedef struct ui_frame {
    ui_node **backups;
    size_t backupc;
    size_t backupcap;
    size_t backupi;
    ui_node **stack;
    size_t stackc;
    size_t stackcap;
} ui_frame;

typedef struct ui_frame_stack {
    ui_frame **frames;
    size_t len;
    size_t cap;
} ui_frame_stack;

typedef struct ui_text {
    char *text;
    Color color;
    int size;
} ui_text;

typedef struct ui_image {
    const char *image;
    int width;
    int height;
} ui_image;

typedef struct ui_box {
    ui_node *child;
    Color color;
} ui_box;

typedef struct ui_pad {
    ui_node *child;
    int px;
    int py;
} ui_pad;

typedef struct ui_align {
    ui_node *child;
    double x;
    double y;
    int width;
    int height;
} ui_align;

typedef struct ui_button_widget {
    ui_button *button;
    ui_node *child;
} ui_button_widget;

typedef struct ui_scrollable_widget {
    ui_scrollable *scrollable;
    ui_node *child;
} ui_scrollable_widget;

typedef struct ui_input_widget {
    ui_input *input;
    int width;
    int height;
} ui_input_widget;

typedef struct ui_slider_widget {
    ui_slider *slider;
    int width;
    int height;
} ui_slider_widget;

typedef struct ui_translate {
    ui_node *child;
    int x;
    int y;
} ui_translate;

typedef struct ui_tooltip {
    ui_node *child;
    const char *name;
    int nameSize;
    const char *description;
    int descSize;
    int maxLineLen;
    char *lineBuffer;
} ui_tooltip;

#define UI_TEXT 0
#define UI_IMAGE 1
#define UI_SPACING 2
#define UI_BUTTON 3
#define UI_SLIDER 4
#define UI_INPUT 5
#define UI_SCROLLABLE 6
#define UI_BOX 7
#define UI_PAD 8
#define UI_ALIGN 9
#define UI_ROW 10
#define UI_COLUMN 11
#define UI_STACK 12
#define UI_TRANSLATE 13
#define UI_TOOLTIP 14

typedef struct ui_node {
    size_t tag;
    union {
        ui_text *text;
        ui_image *image;
        int spacing;
        ui_button_widget *button;
        ui_slider_widget *slider;
        ui_input_widget *input;
        ui_scrollable_widget *scrollable;
        ui_box *box;
        ui_pad *pad;
        ui_align *align;
        ui_frame *row;
        ui_frame *column;
        ui_frame *stack;
        ui_translate *translate;
        ui_tooltip *tooltip;
    };
} ui_node;

static void ui_destroyNode(ui_node *node) {
    // TODO: actually delete stuff and not just leak memory
    free(node);
}


static int ui_widthOf(ui_node *node) {
    if (node->tag == UI_TEXT) {
        Font font = font_get();
        return MeasureTextEx(font, node->text->text, node->text->size, (float)node->text->size / font.baseSize).x;
    } else if (node->tag == UI_IMAGE) {
        return node->image->width;
    } else if (node->tag == UI_SPACING) {
        return node->spacing;
    } else if (node->tag == UI_BUTTON) {
        return ui_widthOf(node->button->child);
    } else if (node->tag == UI_SLIDER) {
        return node->slider->width;
    } else if (node->tag == UI_INPUT) {
        return node->input->width;
    } else if (node->tag == UI_SCROLLABLE) {
        return node->scrollable->scrollable->width;
    } else if (node->tag == UI_BOX) {
        return ui_widthOf(node->box->child);
    } else if (node->tag == UI_PAD) {
        return node->pad->px*2 + ui_widthOf(node->pad->child);
    } else if (node->tag == UI_ALIGN) {
        return node->align->width;
    } else if (node->tag == UI_ROW) {
        int maxwidth = 0;
        for (int i = 0; i < node->row->stackc; ++i) {
            int width = ui_widthOf(node->row->stack[i]);
            maxwidth += width;
        }
        return maxwidth;
    } else if (node->tag == UI_COLUMN) {
        int maxwidth = 0;
        for (int i = 0; i < node->row->stackc; ++i) {
            int width = ui_widthOf(node->row->stack[i]);
            maxwidth = (width > maxwidth) ? width : maxwidth;
        }
        return maxwidth;
    } else if (node->tag == UI_STACK) {
        return 0;
    } else if (node->tag == UI_TRANSLATE) {
        return ui_widthOf(node->translate->child);
    } else if(node->tag == UI_TOOLTIP) {
        return ui_widthOf(node->tooltip->child);
    }
    
    return 0;
}

static int ui_heightOf(ui_node *node) {
    if (node->tag == UI_TEXT) {
        Font font = font_get();
        return MeasureTextEx(font, node->text->text, node->text->size, (float)node->text->size / font.baseSize).y;
    } else if (node->tag == UI_IMAGE) {
        return node->image->height;
    } else if (node->tag == UI_SPACING) {
        return node->spacing;
    } else if (node->tag == UI_BUTTON) {
        return ui_heightOf(node->button->child);
    } else if (node->tag == UI_SLIDER) {
        return node->slider->height;
    } else if (node->tag == UI_INPUT) {
        return node->input->height;
    } else if (node->tag == UI_SCROLLABLE) {
        return node->scrollable->scrollable->height;
    } else if (node->tag == UI_BOX) {
        return ui_heightOf(node->box->child);
    } else if (node->tag == UI_PAD) {
        return node->pad->py*2 + ui_heightOf(node->pad->child);
    } else if (node->tag == UI_ALIGN) {
        return node->align->height;
    } else if (node->tag == UI_ROW) {
        int maxheight = 0;
        for (int i = 0; i < node->row->stackc; ++i) {
            int height = ui_heightOf(node->row->stack[i]);
            maxheight = (height > maxheight) ? height : maxheight;
        }
        return maxheight;
    } else if (node->tag == UI_COLUMN) {
        int maxheight = 0;
        for (int i = 0; i < node->row->stackc; ++i) {
            int height = ui_heightOf(node->row->stack[i]);
            maxheight += height;
        }
        return maxheight;
    } else if (node->tag == UI_STACK) {
        return 0;
    } else if (node->tag == UI_TRANSLATE) {
        return ui_heightOf(node->translate->child);
    } else if(node->tag == UI_TOOLTIP) {
        return ui_heightOf(node->tooltip->child);
    }
    
    return 0;
}

static void ui_drawFrame(ui_frame *frame, int x, int y, int mx, int my);

static void ui_drawNode(ui_node *node, int x, int y) {
    if(node->tag == UI_IMAGE) {
        Texture texture = textures_get(node->image->image);
        int width = node->image->width;
        int height = node->image->height;

        Rectangle src = {0, 0, texture.width, texture.height};
        Rectangle dest = {x, y, width, height};
        Vector2 origin = {0, 0};

        DrawTexturePro(texture, src, dest, origin, 0, WHITE);
        return;
    }

    if(node->tag == UI_TEXT) {
        const char *text = node->text->text;
        int size = node->text->size;
        Color color = node->text->color;
        Font font = font_get();

        Vector2 pos = {x, y};
        float spacing = ((float)size) / font.baseSize;
        DrawTextEx(font, text, pos, size, spacing, color);
        return;
    }

    if(node->tag == UI_INPUT) {
        Font font = font_get();
        ui_input *input = node->input->input;
        const char *text = input->text;
        int size = node->text->size;
        int width = node->input->width;
        if(width == 0) {
            width = input->maxlen * MeasureText("A", size);
        }
        size_t maxDisplayLen = 0;
        int curwidth = 0;
        for(size_t i = 0; i < input->textlen; i++) {
            char sc[2] = {text[i], '\0'};
            int w = MeasureText(sc, size);
            curwidth += w;
            if(curwidth > width) break;
            maxDisplayLen++;
        }
        static char buffer[1024];
        size_t len = input->textlen;
        if(len > maxDisplayLen) len = maxDisplayLen;
        if(len > input->maxlen) len = input->maxlen;
        strncpy(buffer, input->text, len);
        Color color = WHITE;

        Vector2 pos = {x, y};
        float spacing = ((float)size) / font.baseSize;
        DrawTextEx(font, buffer, pos, node->input->height, spacing, color);
        return;
    }

    if(node->tag == UI_PAD) {
        ui_drawNode(node->pad->child, x + node->pad->px, y + node->pad->py);
        return;
    }

    if(node->tag == UI_BOX) {
        DrawRectangle(x, y, ui_widthOf(node->box->child), ui_heightOf(node->box->child), node->box->color);
        ui_drawNode(node->box->child, x, y);
        return;
    }

    if(node->tag == UI_BUTTON) {
        ui_drawNode(node->button->child, x, y);
        return;
    }

    if(node->tag == UI_TRANSLATE) {
        ui_drawNode(node->translate->child, x + node->translate->x, y + node->translate->y);
        return;
    }

    if(node->tag == UI_SCROLLABLE) {

    }

    if(node->tag == UI_ROW) {
        ui_drawFrame(node->row, x, y, 1, 0);
        return;
    }
    
    if(node->tag == UI_COLUMN) {
        ui_drawFrame(node->column, x, y, 0, 1);
        return;
    }
    
    if(node->tag == UI_STACK) {
        ui_drawFrame(node->stack, x, y, 0, 0);
        return;
    }

    if(node->tag == UI_ALIGN) {
        ui_align *align = node->align;
        int offX = align->width * align->x - ui_widthOf(align->child) * align->x;
        int offY = align->height * align->y - ui_heightOf(align->child) * align->y;
        ui_drawNode(align->child, x + offX, y + offY);
        return;
    }

    if(node->tag == UI_TOOLTIP) {
        ui_tooltip *tooltip = node->tooltip;
        ui_drawNode(tooltip->child, x, y);
        int childWidth = ui_widthOf(tooltip->child);
        int childHeight = ui_heightOf(tooltip->child);
        int mx = GetMouseX();
        int my = GetMouseY();
        if(mx < x || my < y || mx > x + childWidth || my > y + childHeight) {
            return;
        }
        int maxWidth = MeasureText(tooltip->name, tooltip->nameSize);
        int spacing = tooltip->descSize / 5;
        int height = tooltip->nameSize;

        {
            // Loop 1: Measurements
            int lineOff = 0;
            int lineLen = 0;
            const char *desc = tooltip->description;
            char *buffer = tooltip->lineBuffer;

            while(true) {
                if(desc[lineOff+lineLen] == '\0') goto lineDone;
                if(desc[lineOff+lineLen] == '\n') goto lineDone;
                if(lineLen == tooltip->maxLineLen) goto lineDone;

                lineLen++;
                continue;

            lineDone:
                memcpy(buffer, desc + lineOff, lineLen * sizeof(char));
                buffer[lineLen] = '\0';
                int lineWidth = MeasureText(buffer, tooltip->descSize);
                if(lineWidth > maxWidth) maxWidth = lineWidth;
                height += spacing + tooltip->descSize;
                if(desc[lineOff+lineLen] == '\0') break;
                lineOff += lineLen + 1;
                lineLen = 0;
            }
        }

        // Draw title
        if(x + maxWidth > GetScreenWidth()) {
            x = GetScreenWidth() - maxWidth;
        }
        DrawText(tooltip->name, x, y - height, tooltip->nameSize, WHITE);
        {
            // Loop 2: Rendering 
            int lineOff = 0;
            int lineLen = 0;
            const char *desc = tooltip->description;
            char *buffer = tooltip->lineBuffer;
            int offY = 0;

            while(true) {
                if(desc[lineOff+lineLen] == '\0') goto lineDone2;
                if(desc[lineOff+lineLen] == '\n') goto lineDone2;
                if(lineLen == tooltip->maxLineLen) goto lineDone2;

                lineLen++;
                continue;

            lineDone2:
                memcpy(buffer, desc + lineOff, lineLen * sizeof(char));
                buffer[lineLen] = '\0';
                offY += spacing + tooltip->descSize;
                DrawText(buffer, x, y - height + offY, tooltip->descSize, WHITE);
                if(desc[lineOff+lineLen] == '\0') break;
                lineOff += lineLen + 1;
                lineLen = 0;
            }
        }
        return;
    }
}

static void ui_updateFrame(ui_frame *frame, int x, int y, int mx, int my, double delta);

static void ui_updateNode(ui_node *node, int x, int y, double delta) {
start:
    if(node->tag == UI_ROW) {
        ui_updateFrame(node->row, x, y, 1, 0, delta);
        return;
    }
    if(node->tag == UI_COLUMN) {
        ui_updateFrame(node->row, x, y, 0, 1, delta);
        return;
    }
    if(node->tag == UI_STACK) {
        ui_updateFrame(node->row, x, y, 0, 0, delta);
        return;
    }

    if(node->tag == UI_PAD) {
        // avoiding recursion as much as possible
        x += node->pad->px;
        y += node->pad->py;
        node = node->pad->child;
        goto start;  
    }

    if(node->tag == UI_BOX) {
        node = node->box->child;
        goto start;
    }

    if(node->tag == UI_BUTTON) {
        ui_button *button = node->button->button;
        int width = ui_widthOf(node->button->child);
        int height = ui_heightOf(node->button->child);
        button->wasClicked = button->clicked;
        button->clicked = false;
        int mx = GetMouseX();
        int my = GetMouseY();
        button->hovered = mx >= x && my >= y && mx <= x + width && my <= y + height;
        if((IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) && button->hovered) {
            button->clicked = true;
            button->pressTime += delta;
            button->rightClick = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        } else {
            button->pressTime = 0;
        }
        node = node->button->child;
        goto start;
    }

    if(node->tag == UI_TRANSLATE) {
        x += node->translate->x;
        y += node->translate->y;
        node = node->translate->child;
        goto start;
    }
    
    if(node->tag == UI_ALIGN) {
        ui_align *align = node->align;
        int offX = align->width * align->x - ui_widthOf(align->child) * align->x;
        int offY = align->height * align->y - ui_heightOf(align->child) * align->y;
        x += offX;
        y += offY;
        node = align->child;
        goto start;
    }

    if(node->tag == UI_TOOLTIP) {
        node = node->tooltip->child;
        goto start;
    }
}

static int ui_frameAbsorbs(ui_frame *frame, int x, int y, int px, int py, int mx, int my);

static int ui_nodeAbsorbs(ui_node *node, int x, int y, int px, int py) {
    if(node->tag == UI_ROW) {
        return ui_frameAbsorbs(node->row, x, y, px, py, 1, 0);
    }
    if(node->tag == UI_COLUMN) {
        return ui_frameAbsorbs(node->row, x, y, px, py, 0, 1);
    }
    if(node->tag == UI_STACK) {
        return ui_frameAbsorbs(node->row, x, y, px, py, 0, 0);
    }
    if(node->tag == UI_PAD) {
        return ui_nodeAbsorbs(node->pad->child, x + node->pad->px, y + node->pad->py, px, py);
    }
    if(node->tag == UI_BUTTON) {
        return px >= x && py >= y && px <= x + ui_widthOf(node->button->child) && py <= y + ui_heightOf(node->button->child);
    }
    if(node->tag == UI_BOX) {
        return px >= x && py >= y && px <= x + ui_widthOf(node->box->child) && py <= y + ui_heightOf(node->box->child);
    }
    if(node->tag == UI_TRANSLATE) {
        return ui_nodeAbsorbs(node->translate->child, x + node->translate->x, y + node->translate->y, px, py);
    }
    if(node->tag == UI_ALIGN) {
        ui_align *align = node->align;
        int offX = align->width * align->x - ui_widthOf(align->child) * align->x;
        int offY = align->height * align->y - ui_heightOf(align->child) * align->y;
        return ui_nodeAbsorbs(align->child, x + offX, y + offY, px, py);
    }
    if(node->tag == UI_TOOLTIP) {
        return ui_nodeAbsorbs(node->tooltip->child, x, y, px, py);
    }
    return false;
}


static void ui_drawFrame(ui_frame *frame, int x, int y, int mx, int my) {
    for(size_t i = 0; i < frame->stackc; i++) {
        ui_node *node = frame->stack[i];
        ui_drawNode(node, x, y);
        x += mx * ui_widthOf(node);
        y += my * ui_heightOf(node);
    }
}

static int ui_frameAbsorbs(ui_frame *frame, int x, int y, int px, int py, int mx, int my) {
    for(size_t i = 0; i < frame->stackc; i++) {
        ui_node *node = frame->stack[i];
        if(ui_nodeAbsorbs(node, x, y, px, py)) return true;
        x += mx * ui_widthOf(node);
        y += my * ui_heightOf(node);
    }
    return false;
}

static void ui_updateFrame(ui_frame *frame, int x, int y, int mx, int my, double delta) {
    for(size_t i = 0; i < frame->stackc; i++) {
        ui_node *node = frame->stack[i];
        ui_updateNode(node, x, y, delta);
        x += mx * ui_widthOf(node);
        y += my * ui_heightOf(node);
    }
}


static ui_frame_stack ui_stack = {NULL, 0, 0};

ui_frame *tsc_ui_newFrame() {
    ui_frame *frame = malloc(sizeof(ui_frame));
    frame->stackcap = 20;
    frame->backupcap = 20;
    frame->stackc = 0;
    frame->stackcap = 20;
    frame->stack = malloc(sizeof(ui_node *) * frame->stackcap);
    frame->backupc = 0;
    frame->backupi = 0;
    frame->backupcap = 20;
    frame->backups = malloc(sizeof(ui_node *) * frame->backupcap);
    return frame;
}

void tsc_ui_destroyFrame(ui_frame *frame) {
    for(size_t i = 0; i < frame->backupc; i++) {
        ui_destroyNode(frame->backups[i]);
    }
    free(frame->backups);
    free(frame->stack);
    free(frame);
}

void tsc_ui_pushFrame(ui_frame *frame) {
    tsc_ui_bringBackFrame(frame);
    tsc_ui_reset();
}

void tsc_ui_bringBackFrame(ui_frame *frame) {
    if(ui_stack.len == ui_stack.cap) {
        if(ui_stack.cap == 0) ui_stack.cap = 5;
        ui_stack.cap *= 2;
        ui_stack.frames = realloc(ui_stack.frames, sizeof(ui_frame *) * ui_stack.cap);
    }
    size_t idx = ui_stack.len++;
    ui_stack.frames[idx] = frame;
}

void tsc_ui_popFrame() {
    if(ui_stack.len == 0) return;
    ui_stack.len--;
}

static ui_frame *tsc_ui_topFrame() {
    if(ui_stack.len == 0) return NULL;
    return ui_stack.frames[ui_stack.len-1];
}

void tsc_ui_reset() {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    // Super easy reset.
    // This better work, or else, I will fucking nuke everything
    frame->stackc = 0;
    frame->backupi = 0;
}

static ui_node *tsc_ui_askFrame(ui_frame *frame, size_t type) {
    if(frame->backupi == frame->backupc) {
        return NULL;
    }
    size_t idx = frame->backupi;
    ui_node *inThere = frame->backups[idx];
    if(inThere == NULL) return NULL;
    if(inThere->tag != type) {
        ui_destroyNode(inThere);
        frame->backups[idx] = NULL;
        return NULL;
    }
    frame->backupi++;
    return inThere;
}

static void tsc_ui_giveFrame(ui_frame *frame, ui_node *node) {
    if(frame->stackc == frame->stackcap) {
        frame->stackcap *= 2;
        frame->stack = realloc(frame->stack, sizeof(ui_node *) * frame->stackcap);
    }
    frame->stack[frame->stackc++] = node;
}

static ui_node *tsc_ui_takeFromFrame(ui_frame *frame) {
    if(frame->stackc == 0) return NULL;
    frame->stackc--;
    return frame->stack[frame->stackc];
}

static void tsc_ui_backupInFrame(ui_frame *frame, ui_node *node) {
    if(frame->backupi < frame->backupc) {
        if(frame->backups[frame->backupi] != NULL) {
            ui_destroyNode(frame->backups[frame->backupi]);
        }
        frame->backups[frame->backupi] = node;
        frame->backupi++;
        return;
    }
    if(frame->backupc == frame->backupcap) {
        frame->backupcap *= 2;
        frame->backups = realloc(frame->backups, sizeof(ui_node *) * frame->backupcap);
    }
    size_t idx = frame->backupc++;
    frame->backups[idx] = node;
    frame->backupi++;
}

// Super high-level stuff

void tsc_ui_update(double delta) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_updateFrame(frame, 0, 0, 0, 0, delta);
}

void tsc_ui_render() {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_drawFrame(frame, 0, 0, 0, 0);
}

int tsc_ui_absorbedPointer(int x, int y) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return false;
    return ui_frameAbsorbs(frame, 0, 0, x, y, 0, 0);
}

// States

ui_button *tsc_ui_newButtonState() {
    ui_button *state = malloc(sizeof(ui_button));
    state->color = GetColor(0x5519AAFF);
    state->clicked = false;
    state->wasClicked = false;
    state->pressTime = 0;
    state->longPressTime = 0.5;
    return state;
}

ui_input *tsc_ui_newInputState(const char *placeholder, size_t maxlen, size_t flags) {
    ui_input *state = malloc(sizeof(ui_input));
    state->placeholder = placeholder;
    state->text = tsc_strdup("");
    state->textlen = strlen(state->text);
    // No upper limit.
    state->maxlen = maxlen;
    state->flags = flags;
    return state;
}

ui_slider *tsc_ui_newSliderState(double min, double max) {
    ui_slider *state = malloc(sizeof(ui_slider));
    state->segments = 0;
    state->min = min;
    state->max = max;
    state->value = min;
    return state;
}

ui_scrollable *tsc_ui_newScrollableState() {
    ui_scrollable *state = malloc(sizeof(ui_scrollable));
    state->width = 0;
    state->height = 0;
    state->offx = 0;
    state->offy = 0;
    // state->texture is uninitialized because fuck you
    return state;
}

static void tsc_ui_resizeScrollable(ui_scrollable *scrollable, int width, int height) {
    if(scrollable->width * scrollable->height != 0) {
        UnloadRenderTexture(scrollable->texture);
    }
    double wprogress = ((double)scrollable->offx) / ((double)scrollable->width);
    double hprogress = ((double)scrollable->offy) / ((double)scrollable->height);
    scrollable->texture = LoadRenderTexture(width, height);
    scrollable->offx = wprogress * width;
    scrollable->offy = hprogress * height;
    scrollable->width = width;
    scrollable->height = height;
}

void tsc_ui_clearButtonState(ui_button *button) {
    button->wasClicked = false;
    button->clicked = false;
    button->pressTime = 0;
}

void tsc_ui_clearInputState(ui_input *input) {
    free(input->text);
    input->text = tsc_strdup("");
    input->textlen = strlen(input->text);
}

void tsc_ui_clearSliderState(ui_slider *slider) {
    slider->value = slider->min;
}

void tsc_ui_clearScrollableState(ui_scrollable *scrollable) {
    scrollable->offx = 0;
    scrollable->offy = 0;
    // Everything else is preserved in hopes of reusing the texture
}

void tsc_ui_image(const char *id, int width, int height) {
    id = tsc_strintern(id);
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *node = tsc_ui_askFrame(frame, UI_IMAGE);
    if(node != NULL) {
        node->image->image = id;
        node->image->width = width;
        node->image->height = height;
        tsc_ui_giveFrame(frame, node);
        return;
    }
    node = malloc(sizeof(ui_node));
    ui_image *image = malloc(sizeof(ui_image));
    image->image = id;
    image->width = width;
    image->height = height;
    node->tag = UI_IMAGE;
    node->image = image;
    tsc_ui_backupInFrame(frame, node);
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_text(const char *text, int size, Color color) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *node = tsc_ui_askFrame(frame, UI_TEXT);
    if(node != NULL) {
        free(node->text->text);
        node->text->text = tsc_strdup(text);
        node->text->size = size;
        node->text->color = color;
        tsc_ui_giveFrame(frame, node);
        return;
    }
    node = malloc(sizeof(ui_node));
    ui_text *uitext = malloc(sizeof(ui_text));
    uitext->text = tsc_strdup(text);
    uitext->size = size;
    uitext->color = color;
    node->tag = UI_TEXT;
    node->text = uitext;
    tsc_ui_backupInFrame(frame, node);
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_space(int amount) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *node = tsc_ui_askFrame(frame, UI_SPACING);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        tsc_ui_backupInFrame(frame, node);
    }
    node->tag = UI_SPACING;
    node->spacing = amount;
    tsc_ui_giveFrame(frame, node);
}

int tsc_ui_button(ui_button *state) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return tsc_ui_checkbutton(state);
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_BUTTON);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_BUTTON;
        ui_button_widget *button = malloc(sizeof(ui_button_widget));
        button->child = child;
        button->button = state;
        node->button = button;
        tsc_ui_backupInFrame(frame, node);
        tsc_ui_giveFrame(frame, node);
        return tsc_ui_checkbutton(state);
    }

    node->button->child = child;
    node->button->button = state;
    tsc_ui_giveFrame(frame, node);
    return tsc_ui_checkbutton(state);
}

int tsc_ui_checkbutton(ui_button *state) {
    if(state->rightClick) {
        if(state->clicked && !state->wasClicked) {
            return UI_BUTTON_RIGHTCLICK;
        }
        if(!state->clicked && state->wasClicked && state->pressTime < state->longPressTime) {
            return UI_BUTTON_RIGHTPRESS;
        }
        if(!state->clicked && state->wasClicked && state->pressTime >= state->longPressTime) {
            return UI_BUTTON_RIGHTLONGPRESS;
        }
    } else {
        if(state->clicked && !state->wasClicked) {
            return UI_BUTTON_CLICK;
        }
        if(!state->clicked && state->wasClicked && state->pressTime < state->longPressTime) {
            return UI_BUTTON_PRESS;
        }
        if(!state->clicked && state->wasClicked && state->pressTime >= state->longPressTime) {
            return UI_BUTTON_LONGPRESS;
        }
    }
    return state->hovered ? UI_BUTTON_HOVER : 0;
}

const char *tsc_ui_input(ui_input *state, int width, int height) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return state->text;
    ui_node *node = tsc_ui_askFrame(frame, UI_BUTTON);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_BUTTON;
        ui_input_widget *input = malloc(sizeof(ui_input_widget));
        input->input = state;
        input->width = width;
        input->height = height;
        node->input = input;
        tsc_ui_backupInFrame(frame, node);
        tsc_ui_giveFrame(frame, node);
        return state->text;
    }

    ui_input_widget *input = node->input;
    input->input = state;
    input->width = width;
    input->height = height;
    tsc_ui_giveFrame(frame, node);
    return state->text;
}

double tsc_ui_slider(ui_slider *state, int width, int height, int thickness) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return state->value;
    ui_node *node = tsc_ui_askFrame(frame, UI_BUTTON);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_BUTTON;
        ui_slider_widget *slider = malloc(sizeof(ui_slider_widget));
        slider->slider = state;
        slider->width = width;
        slider->height = height;
        node->slider = slider;
        tsc_ui_backupInFrame(frame, node);
        tsc_ui_giveFrame(frame, node);
        return state->value;
    }

    ui_slider_widget *slider = node->slider;
    slider->slider = state;
    slider->width = width;
    slider->height = height;
    tsc_ui_giveFrame(frame, node);
    return state->value;
}

void tsc_ui_pushRow() {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *node = tsc_ui_askFrame(frame, UI_ROW);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_ROW;
        node->row = tsc_ui_newFrame();
        tsc_ui_backupInFrame(frame, node);
    }
    tsc_ui_giveFrame(frame, node);
    tsc_ui_pushFrame(node->row);
    tsc_ui_reset();
}

void tsc_ui_finishRow() {
    tsc_ui_popFrame();
}

void tsc_ui_translate(int x, int y) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_TRANSLATE);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_TRANSLATE;
        node->translate = malloc(sizeof(ui_translate));
        tsc_ui_backupInFrame(frame, node);
    }
    node->translate->x = x;
    node->translate->y = y;
    node->translate->child = child;
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_pad(int x, int y) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_PAD);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_PAD;
        node->pad = malloc(sizeof(ui_pad));
        tsc_ui_backupInFrame(frame, node);
    }
    node->pad->px = x;
    node->pad->py = y;
    node->pad->child = child;
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_align(float x, float y, int width, int height) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_ALIGN);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_ALIGN;
        node->align = malloc(sizeof(ui_align));
        tsc_ui_backupInFrame(frame, node);
    }
    node->align->x = x;
    node->align->y = y;
    node->align->width = width;
    node->align->height = height;
    node->align->child = child;
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_box(Color background) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_BOX);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_BOX;
        node->box = malloc(sizeof(ui_box));
        tsc_ui_backupInFrame(frame, node);
    }
    node->box->color = background;
    node->box->child = child;
    tsc_ui_giveFrame(frame, node);
}

void tsc_ui_tooltip(const char *name, int nameSize, const char *description, int descSize, int maxLineLen) {
    ui_frame *frame = tsc_ui_topFrame();
    if(frame == NULL) return;
    ui_node *child = tsc_ui_takeFromFrame(frame);
    ui_node *node = tsc_ui_askFrame(frame, UI_TOOLTIP);
    if(node == NULL) {
        node = malloc(sizeof(ui_node));
        node->tag = UI_TOOLTIP;
        node->tooltip = malloc(sizeof(ui_tooltip));
        node->tooltip->child = child;
        node->tooltip->name = name;
        node->tooltip->nameSize = nameSize;
        node->tooltip->description = description;
        node->tooltip->descSize = descSize;
        node->tooltip->maxLineLen = maxLineLen;
        node->tooltip->lineBuffer = malloc(sizeof(char) * (maxLineLen + 1));
        tsc_ui_backupInFrame(frame, node);
        tsc_ui_giveFrame(frame, node);
        return;
    }
    node->tooltip->child = child;
    node->tooltip->name = name;
    node->tooltip->nameSize = nameSize;
    node->tooltip->description = description;
    node->tooltip->descSize = descSize;
    if(node->tooltip->maxLineLen != maxLineLen) {
        node->tooltip->maxLineLen = maxLineLen;
        node->tooltip->lineBuffer = realloc(node->tooltip->lineBuffer, sizeof(char) * (maxLineLen + 1));
    }
    tsc_ui_giveFrame(frame, node);
}
