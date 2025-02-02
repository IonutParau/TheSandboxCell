#include "ui2.h"
#include "../utils.h"
#include "resources.h"
#include <stdlib.h>

double tsc_ui_scale() {
    int height = GetScreenHeight();
    return ((double)height) / 1080; // yup, yup, yup
}

tsc_ui_node tsc_ui_baseNode(short nodeType) {
    return (tsc_ui_node) {1, nodeType, 0, NULL};
}

int tsc_ui_typeOf(tsc_ui_node *node) {
    return node->nodeType;
}

int tsc_ui_childCountOf(tsc_ui_node *node) {
    return node->childCount;
}

tsc_ui_node **tsc_ui_childrenOf(tsc_ui_node *node) {
    return node->subnodes;
}

void tsc_ui_addChild(tsc_ui_node *parent, tsc_ui_node *node) {
    tsc_ui_moveChild(parent, tsc_ui_retain(node));
}

void tsc_ui_moveChild(tsc_ui_node *parent, tsc_ui_node *node) {
    int idx = parent->childCount++;
    parent->subnodes = realloc(parent->subnodes, sizeof(tsc_ui_node *) * parent->childCount);
    parent->subnodes[idx] = node;
}
void tsc_ui_removeChild(tsc_ui_node *parent, tsc_ui_node *node) {
    int j = 0;
    for(int i = 0; i < parent->childCount; i++) {
        tsc_ui_node *pnode = parent->subnodes[i];
        if(pnode != node) {
            parent->subnodes[j] = pnode;
            j++;
        }
    }
    parent->childCount = j;
}

tsc_ui_node *tsc_ui_retain(tsc_ui_node *node) {
    node->refc++;
    return node;
}

void tsc_ui_destroy(tsc_ui_node *node) {
    node->refc--;
    if(node->refc > 0) return;
    // TODO: all the destructors
}

tsc_ui_context tsc_ui_rootContext(int width, int height) {
    float theScale = tsc_ui_scale();
    return (tsc_ui_context) {0, 0, width / theScale, height / theScale};
}

int tsc_ui_widthOf(tsc_ui_node *node, tsc_ui_context ctx) {
    if(node->nodeType == TSC_UI_TEXT) {
        tsc_ui_text *text = (void *)node;
        Font font = font_get();
        if(text->font != NULL) font = *text->font;
        float scale = tsc_ui_scale();
        return MeasureTextEx(font, text->text, text->fontSize * scale, text->fontSize * scale / 10).x;
    }
    return 0;
}

int tsc_ui_heightOf(tsc_ui_node *node, tsc_ui_context ctx) {
    if(node->nodeType == TSC_UI_TEXT) {
        tsc_ui_text *text = (void *)node;
        Font font = font_get();
        if(text->font != NULL) font = *text->font;
        float scale = tsc_ui_scale();
        return MeasureTextEx(font, text->text, text->fontSize * scale, text->fontSize * scale / 10).y;
    }
    return 0;
}

void tsc_ui_draw(tsc_ui_node *node, tsc_ui_context ctx) {
    if(node->nodeType == TSC_UI_TEXT) {
        tsc_ui_text *text = (void *)node;
        Font font = font_get();
        if(text->font != NULL) font = *text->font;
        float scale = tsc_ui_scale();
        Vector2 pos = (Vector2) {ctx.x, ctx.y};
        DrawTextEx(font, text->text, pos, text->fontSize * scale, text->fontSize * scale / 10, text->color);
    }
    if(node->nodeType == TSC_UI_TRANSFORM) {
        tsc_ui_transform *trans = (void *)node;
        ctx.x += trans->shiftx;
        ctx.y += trans->shifty;
    }
    for(int i = 0; i < node->childCount; i++) {
        tsc_ui_draw(node->subnodes[i], ctx);
    }
}

void tsc_ui_update(tsc_ui_node *node, tsc_ui_context ctx, double dt) {
    for(int i = 0; i < node->childCount; i++) {
        tsc_ui_update(node->subnodes[i], ctx, dt);
    }
}

bool tsc_ui_absorbs(tsc_ui_node *node, tsc_ui_context ctx, int mx, int my) {
    for(int i = 0; i < node->childCount; i++) {
        if(tsc_ui_absorbs(node->subnodes[i], ctx, mx, my)) {
            return true;
        }
    }
    return false;
}

tsc_ui_text *tsc_ui_newText(const char *text, int fontSize, Color color, Font *font) {
    tsc_ui_text *ntext = malloc(sizeof(tsc_ui_text));
    ntext->_node = tsc_ui_baseNode(TSC_UI_TEXT);
    ntext->font = font;
    ntext->color = color;
    ntext->fontSize = fontSize;
    ntext->text = tsc_strdup(text);
    return ntext;
}

tsc_ui_conditional *tsc_ui_newConditional(bool (*check)(void *udata), void *udata) {
    tsc_ui_conditional *ncond = malloc(sizeof(tsc_ui_conditional));
    ncond->_node = tsc_ui_baseNode(TSC_UI_CONDITIONAL);
    ncond->check = check;
    ncond->udata = udata;
    return ncond;
}

tsc_ui_button *tsc_ui_newButton(tsc_ui_node *child) {
    tsc_ui_button *nbtn = malloc(sizeof(tsc_ui_button));
    nbtn->_node = tsc_ui_baseNode(TSC_UI_BUTTON);
    nbtn->pressTime = 0;
    nbtn->longPressTime = 0.2;
    nbtn->wasClicked = false;
    nbtn->clicked = false;
    nbtn->rightClick = false;
    nbtn->hovered = false;
    return nbtn;
}

bool tsc_ui_buttonPressed(tsc_ui_button *button, int mbtn, bool isLong) {
    if(button->clicked) return false; // still counting press time
    if(!button->wasClicked) return false; // a frame too late, kys
    if(isLong && button->pressTime < button->longPressTime) return false;
    if(!isLong && button->pressTime >= button->longPressTime) return false;
    if(mbtn == MOUSE_BUTTON_LEFT) {
        return !button->rightClick;
    }
    if(mbtn == MOUSE_BUTTON_RIGHT) {
        return button->rightClick;
    }
    return false;
}
