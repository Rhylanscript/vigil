// apps/calculator/calculator.c

#include "app.h"
#include "api.h"
#include "calc.h"
#include "keyboard.h"
#include "font.h"
#include "title_bar.h"
#include "taskbar.h"
#include "calculator_icon.h"

// --- API pointer ---

static const vigil_api_t* api = 0;

// --- layout (pixels) ---

#define KEY_W           96
#define KEY_H           72
#define KEY_GAP         8

#define GRID_COLS       4
#define GRID_ROWS       5

#define DISPLAY_H       72
#define DISPLAY_GAP     16
#define DISPLAY_PAD     12

#define CALC_WIDTH      (GRID_COLS * KEY_W + (GRID_COLS - 1) * KEY_GAP)
#define CALC_HEIGHT     (DISPLAY_H + DISPLAY_GAP + GRID_ROWS * KEY_H + (GRID_ROWS - 1) * KEY_GAP)

#define DISPLAY_BG      GFX_BLACK
#define DIGIT_COLOR     ((gfx_color_t){0x30, 0x30, 0x30})
#define OPERATOR_COLOR  ((gfx_color_t){0x40, 0x40, 0x70})
#define EQUALS_COLOR    ((gfx_color_t){0x30, 0x70, 0x30})
#define CLEAR_COLOR     ((gfx_color_t){0xa0, 0x20, 0x20})

// --- the keys ---

typedef enum {
    CALC_KEY_DIGIT = 0,
    CALC_KEY_OPERATOR,
    CALC_KEY_EQUALS,
    CALC_KEY_NEGATE,
    CALC_KEY_BACKSPACE,
    CALC_KEY_CLEAR
} key_kind_t;

typedef struct {
    const char* label;
    int row;
    int col;
    int span;
    key_kind_t kind;
    int value;
} key_def_t;

static const key_def_t keys[] = {
    { "C",   0, 0, 1, CALC_KEY_CLEAR,       0 },
    { "<",   0, 1, 1, CALC_KEY_BACKSPACE,   0 },
    { "%",   0, 2, 1, CALC_KEY_OPERATOR,    CALC_OP_MOD },
    { "/",   0, 3, 1, CALC_KEY_OPERATOR,    CALC_OP_DIV },

    { "7",   1, 0, 1, CALC_KEY_DIGIT,       7 },
    { "8",   1, 1, 1, CALC_KEY_DIGIT,       8 },
    { "9",   1, 2, 1, CALC_KEY_DIGIT,       9 },
    { "*",   1, 3, 1, CALC_KEY_OPERATOR,    CALC_OP_MUL },

    { "4",   2, 0, 1, CALC_KEY_DIGIT,       4 },
    { "5",   2, 1, 1, CALC_KEY_DIGIT,       5 },
    { "6",   2, 2, 1, CALC_KEY_DIGIT,       6 },
    { "-",   2, 3, 1, CALC_KEY_OPERATOR,    CALC_OP_SUB },

    { "1",   3, 0, 1, CALC_KEY_DIGIT,       1 },
    { "2",   3, 1, 1, CALC_KEY_DIGIT,       2 },
    { "3",   3, 2, 1, CALC_KEY_DIGIT,       3 },
    { "+",   3, 3, 1, CALC_KEY_OPERATOR,    CALC_OP_ADD },

    { "+/-", 4, 0, 1, CALC_KEY_NEGATE,      0 },
    { "0",   4, 1, 1, CALC_KEY_DIGIT,       0 },
    { "=",   4, 2, 2, CALC_KEY_EQUALS,      0 },
};

#define CALC_KEY_COUNT ((int) (sizeof(keys) / sizeof(keys[0])))

// --- state ---

static button_t buttons[CALC_KEY_COUNT];

static uint32_t origin_x = 0;
static uint32_t origin_y = 0;

// --- layout ---

static gfx_color_t color_for(key_kind_t kind) {
    if (kind == CALC_KEY_OPERATOR || kind == CALC_KEY_NEGATE) {
        return OPERATOR_COLOR;
    }
    if (kind == CALC_KEY_EQUALS) {
        return EQUALS_COLOR;
    }
    if (kind == CALC_KEY_CLEAR || kind == CALC_KEY_BACKSPACE) {
        return CLEAR_COLOR;
    }
    return DIGIT_COLOR;
}

static uint32_t band_height(void) {
    uint32_t used = TITLE_BAR_HEIGHT + TASKBAR_HEIGHT;
    if (api->gfx_screen_height() > used) {
        return api->gfx_screen_height() - used;
    }
    return 0;
}

static void compute_layout(void) {
    origin_x = 0;
    if (api->gfx_screen_width() > CALC_WIDTH) {
        origin_x = (api->gfx_screen_width() - CALC_WIDTH) / 2;
    }

    origin_y = TITLE_BAR_HEIGHT;
    if (band_height() > CALC_HEIGHT) {
        origin_y += (band_height() - CALC_HEIGHT) / 2;
    }

    uint32_t grid_top = origin_y + DISPLAY_H + DISPLAY_GAP;

    for (int i = 0; i < CALC_KEY_COUNT; i++) {
        const key_def_t* key = &keys[i];

        buttons[i].x = origin_x + (uint32_t) key->col * (KEY_W + KEY_GAP);
        buttons[i].y = grid_top + (uint32_t) key->row * (KEY_H + KEY_GAP);
        buttons[i].width = (uint32_t) key->span * KEY_W + (uint32_t) (key->span - 1) * KEY_GAP;
        buttons[i].height = KEY_H;
        buttons[i].label = key->label;
        buttons[i].color = color_for(key->kind);
        buttons[i].on_click = 0;
    }
}

// --- drawing ---

static void draw_display(void) {
    api->gfx_fill_rect(origin_x, origin_y, CALC_WIDTH, DISPLAY_H, DISPLAY_BG);
    api->gfx_draw_rect_outline(origin_x, origin_y, CALC_WIDTH, DISPLAY_H, GFX_WHITE);

    const char* text;
    char number[12];
    gfx_color_t color = GFX_WHITE;

    calc_error_t error = calc_error();
    if (error == CALC_ERR_DIV_ZERO) {
        text = "Error: divide by zero";
        color = GFX_RED;
    } else if (error == CALC_ERR_OVERFLOW) {
        text = "Error: number too big";
        color = GFX_RED;
    } else {
        api->kitoa(calc_display_value(), number);
        text = number;
    }

    uint32_t text_width = api->kstrlen(text) * FONT_WIDTH;
    uint32_t text_x = origin_x + CALC_WIDTH - DISPLAY_PAD - text_width;
    uint32_t text_y = origin_y + (DISPLAY_H - FONT_HEIGHT) / 2;
    api->gfx_draw_string(text_x, text_y, text, color);

    char symbol = calc_op_symbol(calc_pending_op());
    if (symbol != 0 && error == CALC_OK) {
        api->gfx_draw_char(origin_x + DISPLAY_PAD, origin_y + DISPLAY_PAD, symbol, GFX_GREEN);
    }
}

static void paint(void) {
    api->gfx_fill_rect(0, TITLE_BAR_HEIGHT, api->gfx_screen_width(), band_height(), GFX_BLACK);

    draw_display();

    for (int i = 0; i < CALC_KEY_COUNT; i++) {
        api->button_draw(&buttons[i]);
    }
}

// --- input ---

static void press_key(const key_def_t* key) {
    switch (key->kind) {
        case CALC_KEY_DIGIT:     calc_press_digit(key->value); break;
        case CALC_KEY_OPERATOR:  calc_press_operator((calc_op_t) key->value); break;
        case CALC_KEY_EQUALS:    calc_press_equals(); break;
        case CALC_KEY_NEGATE:    calc_press_negate(); break;
        case CALC_KEY_BACKSPACE: calc_press_backspace(); break;
        case CALC_KEY_CLEAR:     calc_clear(); break;
    }
}

// --- app_t slots ---

static void calculator_start(void) {
    calc_clear();
    compute_layout();
    paint();
}

static void calculator_redraw(void) {
    compute_layout();
    paint();
}

static void calculator_handle_char(char c) {
    api->mouse_hide_cursor();

    calc_op_t op = calc_op_from_char(c);

    if (c >= '0' && c <= '9') {
        calc_press_digit(c - '0');
    } else if (op != CALC_OP_NONE) {
        calc_press_operator(op);
    } else if (c == '=' || c == '\n') {
        calc_press_equals();
    } else if (c == '\b') {
        calc_press_backspace();
    } else if (c == 'c' || c == 'C' || c == KEY_DELETE) {
        calc_clear();
    } else if (c == 'n' || c == 'N') {
        calc_press_negate();
    } else {
        return;
    }

    draw_display();
}

static void calculator_handle_click(int32_t x, int32_t y) {
    int index = api->button_find_at(buttons, CALC_KEY_COUNT, x, y);
    if (index < 0) {
        return;
    }

    press_key(&keys[index]);
    draw_display();
}

// --- the entry point the loader calls ---

int app_entry(const vigil_api_t* kernel_api, app_t* out_app) {
    if (kernel_api->version != VIGIL_API_VERSION || kernel_api->struct_size < sizeof(vigil_api_t)) {
        return 1;
    }
    api = kernel_api;

    out_app->name = "Calculator";
    out_app->start = calculator_start;
    out_app->redraw = calculator_redraw;
    out_app->on_key = calculator_handle_char;
    out_app->on_click = calculator_handle_click;
    out_app->icon = calculator_icon;
    return 0;
}
