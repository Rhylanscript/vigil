// kernel/ui/prompt.c

#include "prompt.h"
#include "graphics.h"
#include "font.h"
#include "keyboard.h"
#include "taskbar.h"
#include "title_bar.h"

// --- layout (px) ---

#define BOX_WIDTH           400
#define BOX_HEIGHT          100
#define BOX_PAD             16

#define TITLE_Y_OFFSET      10
#define FIELD_Y_OFFSET      36
#define FIELD_HEIGHT        24
#define FIELD_TEXT_PAD      6
#define HINT_Y_OFFSET       72

#define CURSOR_BAR_HEIGHT   2

#define TITLE_MAX           48

#define BOX_COLOR           ((gfx_color_t){0x30, 0x30, 0x30})
#define FIELD_COLOR         GFX_BLACK
#define FIELD_BORDER_COLOR  ((gfx_color_t){0x90, 0x90, 0x90})
#define HINT_COLOR          ((gfx_color_t){0xa0, 0xa0, 0xa0})

// --- state ---

static int active = 0;

static char title[TITLE_MAX];

static char text[PROMPT_BUFFER_SIZE];
static int length = 0;

static int limit = 0;

static const char* blocked = 0;

static uint32_t box_x = 0;
static uint32_t box_y = 0;

// --- helpers ---

static int is_blocked(char c) {
    if (blocked == 0) {
        return 0;
    }
    for (int i = 0; blocked[i] != '\0'; i++) {
        if (blocked[i] == c) {
            return 1;
        }
    }
    return 0;
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

// --- drawing ---

static void draw_field(void) {
    uint32_t field_x = box_x + BOX_PAD;
    uint32_t field_y = box_y + FIELD_Y_OFFSET;
    uint32_t field_w = BOX_WIDTH - 2 * BOX_PAD;

    gfx_fill_rect(field_x, field_y, field_w, FIELD_HEIGHT, FIELD_COLOR);
    gfx_draw_rect_outline(field_x, field_y, field_w, FIELD_HEIGHT, FIELD_BORDER_COLOR);

    uint32_t text_x = field_x + FIELD_TEXT_PAD;
    uint32_t text_y = field_y + (FIELD_HEIGHT - FONT_HEIGHT) / 2;
    gfx_draw_string(text_x, text_y, text, GFX_WHITE);

    gfx_fill_rect(
        text_x + (uint32_t) length * FONT_WIDTH,
        text_y + FONT_HEIGHT - CURSOR_BAR_HEIGHT,
        FONT_WIDTH,
        CURSOR_BAR_HEIGHT,
        GFX_WHITE
    );
}

static void draw_box(void) {
    gfx_fill_rect(box_x, box_y, BOX_WIDTH, BOX_HEIGHT, BOX_COLOR);
    gfx_draw_rect_outline(box_x, box_y, BOX_WIDTH, BOX_HEIGHT, GFX_WHITE);

    gfx_draw_string(box_x + BOX_PAD, box_y + TITLE_Y_OFFSET, title, GFX_WHITE);
    draw_field();
    gfx_draw_string(box_x + BOX_PAD, box_y + HINT_Y_OFFSET, "Enter: OK    Esc: cancel", HINT_COLOR);
}

// --- public functions ---

void prompt_open(const char* title_text, const char* initial_text, int max_length, const char* blocked_chars) {
    int i = 0;
    while (title_text[i] != '\0' && i < TITLE_MAX - 1) {
        title[i] = title_text[i];
        i++;
    }
    title[i] = '\0';

    limit = max_length;
    if (limit > PROMPT_BUFFER_SIZE - 1) {
        limit = PROMPT_BUFFER_SIZE - 1;
    }
    if (limit < 1) {
        limit = 1;
    }

    blocked = blocked_chars;

    length = 0;
    while (initial_text[length] != '\0' && length < limit) {
        text[length] = initial_text[length];
        length++;
    }
    text[length] = '\0';

    place_box();
    active = 1;
    draw_box();
}

int prompt_is_open(void) {
    return active;
}

prompt_result_t prompt_handle_key(char c) {
    if (!active) {
        return PROMPT_CONTINUE;
    }

    if (c == KEY_ESCAPE) {
        active = 0;
        return PROMPT_CANCELLED;
    }

    if (c == '\n') {
        if (length == 0) {
            return PROMPT_CONTINUE;
        }
        active = 0;
        return PROMPT_ACCEPTED;
    }

    if (c == '\b') {
        if (length > 0) {
            length--;
            text[length] = '\0';
            draw_field();
        }
        return PROMPT_CONTINUE;
    }

    if (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR && length < limit && !is_blocked(c)) {
        text[length] = c;
        length++;
        text[length] = '\0';
        draw_field();
    }

    return PROMPT_CONTINUE;
}

const char* prompt_text(void) {
    return text;
}

void prompt_close(void) {
    active = 0;
}
