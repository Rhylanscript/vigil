// kernel/ui/dialog.c

#include "dialog.h"
#include "button.h"
#include "graphics.h"
#include "font.h"
#include "keyboard.h"
#include "taskbar.h"
#include "title_bar.h"

// --- layout (px) ---

#define BOX_WIDTH           400
#define BOX_HEIGHT          120
#define BOX_PAD             16

#define TITLE_Y_OFFSET      10
#define MESSAGE_Y_OFFSET    38
#define BUTTON_Y_OFFSET     72

#define DIALOG_BUTTON_W            112
#define DIALOG_BUTTON_H            32
#define DIALOG_BUTTON_GAP          12

#define TEXT_MAX            48

#define BOX_COLOR           ((gfx_color_t){0x30, 0x30, 0x30})
#define BUTTON_COLOR        ((gfx_color_t){0x40, 0x40, 0x40})
#define MESSAGE_COLOR       ((gfx_color_t){0xa0, 0xa0, 0xa0})

// --- state ---

static int active = 0;

static char title[TEXT_MAX];
static char message[TEXT_MAX];

static button_t buttons[DIALOG_MAX_BUTTONS];
static int button_count = 0;

static uint32_t box_x = 0;
static uint32_t box_y = 0;

// --- helpers ---

static void copy_text(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0' && i < TEXT_MAX - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void place_box(void) {
    uint32_t screen_w = gfx_screen_width();
    uint32_t screen_h = gfx_screen_height();

    box_x = 0;
    if (screen_w > BOX_WIDTH) {
        box_x = (screen_w - BOX_WIDTH) / 2;
    }

    uint32_t band_top = TITLE_BAR_HEIGHT;
    uint32_t band_height = 0;
    if (screen_h > band_top + TASKBAR_HEIGHT) {
        band_height = screen_h - band_top - TASKBAR_HEIGHT;
    }

    box_y = band_top;
    if (band_height > BOX_HEIGHT) {
        box_y = band_top + (band_height - BOX_HEIGHT) / 2;
    }
}

static void place_buttons(const char* const* labels, int count) {
    uint32_t row_width = (uint32_t) count * DIALOG_BUTTON_W + (uint32_t) (count - 1) * DIALOG_BUTTON_GAP;
    uint32_t start_x = box_x + (BOX_WIDTH - row_width) / 2;

    for (int i = 0; i < count; i++) {
        buttons[i].x = start_x + (uint32_t) i * (DIALOG_BUTTON_W + DIALOG_BUTTON_GAP);
        buttons[i].y = box_y + BUTTON_Y_OFFSET;
        buttons[i].width = DIALOG_BUTTON_W;
        buttons[i].height = DIALOG_BUTTON_H;
        buttons[i].label = labels[i];
        buttons[i].color = BUTTON_COLOR;
        buttons[i].on_click = 0;
    }
}

// --- drawing ---

static void draw_box(void) {
    gfx_fill_rect(box_x, box_y, BOX_WIDTH, BOX_HEIGHT, BOX_COLOR);
    gfx_draw_rect_outline(box_x, box_y, BOX_WIDTH, BOX_HEIGHT, GFX_WHITE);

    gfx_draw_string(box_x + BOX_PAD, box_y + TITLE_Y_OFFSET, title, GFX_WHITE);
    gfx_draw_string(box_x + BOX_PAD, box_y + MESSAGE_Y_OFFSET, message, MESSAGE_COLOR);

    for (int i = 0; i < button_count; i++) {
        button_draw(&buttons[i]);
    }
}

// --- public functions ---

void dialog_open(const char* title_text, const char* message_text, const char* const* labels, int count) {
    if (count > DIALOG_MAX_BUTTONS) {
        count = DIALOG_MAX_BUTTONS;
    }
    if (count < 1) {
        return;
    }

    copy_text(title, title_text);
    copy_text(message, message_text);

    place_box();
    place_buttons(labels, count);

    button_count = count;
    active = 1;
    draw_box();
}

int dialog_is_open(void) {
    return active;
}

int dialog_click(int32_t px, int32_t py) {
    if (!active) {
        return DIALOG_NO_PICK;
    }

    int pick = button_find_at(buttons, button_count, px, py);
    if (pick < 0) {
        return DIALOG_NO_PICK;
    }

    active = 0;
    return pick;
}

int dialog_handle_key(char c) {
    if (!active) {
        return DIALOG_NO_PICK;
    }

    if (c == KEY_ESCAPE) {
        active = 0;
        return button_count - 1;
    }
    if (c == '\n') {
        active = 0;
        return 0;
    }

    return DIALOG_NO_PICK;
}

void dialog_close(void) {
    active = 0;
}
