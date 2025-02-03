// UI so good we had to make UI 2
#ifndef TSC_UI2
#define TSC_UI2

#include <stddef.h>
#include <raylib.h>

double tsc_ui_scale();

// MUST BE FIRST FIELD OF ALL NODE TYPES!!!!!!!
typedef struct tsc_ui_node {
    unsigned short refc;
    short nodeType;
    int childCount;
    struct tsc_ui_node **subnodes;
} tsc_ui_node;

tsc_ui_node tsc_ui_baseNode(short nodeType);
int tsc_ui_typeOf(tsc_ui_node *node);
int tsc_ui_childCountOf(tsc_ui_node *node);
tsc_ui_node **tsc_ui_childrenOf(tsc_ui_node *node);
void tsc_ui_addChild(tsc_ui_node *parent, tsc_ui_node *node);
void tsc_ui_moveChild(tsc_ui_node *parent, tsc_ui_node *node);
void tsc_ui_removeChild(tsc_ui_node *parent, tsc_ui_node *node);

tsc_ui_node *tsc_ui_retain(tsc_ui_node *node);
void tsc_ui_destroy(tsc_ui_node *node);

typedef struct tsc_ui_context {
    int x;
    int y;
    int width;
    int height;
} tsc_ui_context;

tsc_ui_context tsc_ui_rootContext(int width, int height);
int tsc_ui_widthOf(tsc_ui_node *node, tsc_ui_context ctx);
int tsc_ui_heightOf(tsc_ui_node *node, tsc_ui_context ctx);
void tsc_ui_draw(tsc_ui_node *node, tsc_ui_context ctx);
void tsc_ui_update(tsc_ui_node *node, tsc_ui_context ctx, double dt);
bool tsc_ui_absorbs(tsc_ui_node *node, tsc_ui_context ctx, int mx, int my);

// widgets

#define TSC_UI_NODE 0
// just use tsc_ui_node

#define TSC_UI_TEXT 1
typedef struct tsc_ui_text {
    tsc_ui_node _node;
    char *text;
    int fontSize;
    Color color;
    Font *font;
} tsc_ui_text;

tsc_ui_text *tsc_ui_newText(const char *text, int fontSize, Color color, Font *font);

#define TSC_UI_CONDITIONAL 2
typedef struct tsc_ui_conditional {
    tsc_ui_node _node;
    void *udata;
    int (*check)(void *udata);
} tsc_ui_conditional;

tsc_ui_conditional *tsc_ui_newConditional(int (*check)(void *udata), void *udata);

#define TSC_UI_BUTTON 3
typedef struct tsc_ui_button {
    tsc_ui_node _node;
    float pressTime;
    float longPressTime;
    bool wasClicked;
    bool clicked;
    bool rightClick;
    bool hovered;
} tsc_ui_button;

tsc_ui_button *tsc_ui_newButton(tsc_ui_node *child);
bool tsc_ui_buttonPressed(tsc_ui_button *button, int mbtn, bool isLong);

#define TSC_UI_SLIDER 4
typedef struct tsc_ui_slider {
    tsc_ui_node _node;
    double value;
    double min;
    double max;
    size_t segments;
    int width;
    int height;
} tsc_ui_slider;

tsc_ui_slider *tsc_ui_newSlider(double min, double max, size_t segments, int width, int height);

#define TSC_UI_TRANSFORM 5
typedef struct tsc_ui_transform {
    tsc_ui_node _node;
    int shiftx;
    int shifty;
} tsc_ui_transform;

tsc_ui_transform *tsc_ui_newTransform(int shiftX, int shiftY, tsc_ui_node *node);

#define TSC_UI_ALIGN 6
typedef struct tsc_ui_align {
    tsc_ui_node _node;
    double xAlign;
    double yAlign;
    int width;
    int height;
} tsc_ui_align;

tsc_ui_align *tsc_ui_newAlign(double xAlign, double yAlign, int width, int height, tsc_ui_node *node);

#define TSC_UI_SCROLLABLE 7
typedef struct tsc_ui_scrollable {
    tsc_ui_node _node;
    int width;
    int height;
    double offset;
} tsc_ui_scrollable;

tsc_ui_scrollable *tsc_ui_newScrollable(int width, int height, tsc_ui_node *node);

#define TSC_UI_INPUT 8
#define TSC_UI_INPUT_TEXT 0
#define TSC_UI_INPUT_SIZE 1
#define TSC_UI_INPUT_NUMBER 2
typedef struct tsc_ui_input {
    tsc_ui_node _node;
    char *buffer;
    int buflen;
    int cursor;
    int bufsize;
    int width;
    int height;
    float blinkTimer;
    bool showsCursor;
    bool submitted;
    char mode;
    char *charset;
} tsc_ui_input;

tsc_ui_input *tsc_ui_newInput(int width, int height, int bufsize, char mode, char *charset);

#define TSC_UI_IMAGE 9
typedef struct tsc_ui_image {
    tsc_ui_node _node;
    Texture texture;
    int width;
    int height;
} tsc_ui_image;

tsc_ui_image *tsc_ui_newImage(Texture texture, int width, int height);

#define TSC_UI_ROW 10
typedef struct tsc_ui_row {
    tsc_ui_node _node;
    int spacing;
} tsc_ui_row;

tsc_ui_row *tsc_ui_newRow(int spacing);

#define TSC_UI_COLUMN 11
typedef struct tsc_ui_column {
    tsc_ui_node _node;
    int spacing;
} tsc_ui_column;

tsc_ui_column *tsc_ui_newColumn(int spacing);

#define TSC_UI_BOX 12
typedef struct tsc_ui_box {
    tsc_ui_node _node;
    Color color;
    int width;
    int height;
} tsc_ui_box;

tsc_ui_box *tsc_ui_newBox(Color color, int width, int height);

#endif
