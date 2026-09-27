// boot/drivers/terminal.c

#include "terminal.h"
#include "graphics.h"
#include "font.h"
#include "timer.h"

#define TERM_FG GFX_WHITE
#define TERM_BG GFX_BLACK

#define CURSOR_BAR_HEIGHT 2
#define CURSOR_BLINK_MS 500

static int cursor_row;
static int cursor_col;

static int text_cols;
static int text_rows;

static int cursor_blink_on;
static uint32_t last_blink_tick;

static uint32_t cell_x(int col) {
    return (uint32_t) col * FONT_WIDTH;
}
static uint32_t cell_y(int row) {
    return (uint32_t) row * FONT_HEIGHT;
}

static void clear_cell(int col, int row) {
    gfx_fill_rect(cell_x(col), cell_y(row), FONT_WIDTH, FONT_HEIGHT, TERM_BG);
}

static void draw_cursor_bar(void) {
    gfx_fill_rect(
        cell_x(cursor_col),
        cell_y(cursor_row) + (FONT_HEIGHT - CURSOR_BAR_HEIGHT),
        FONT_WIDTH,
        CURSOR_BAR_HEIGHT,
        TERM_FG
    );
}
static void erase_cursor_bar(void) {
    gfx_fill_rect(
        cell_x(cursor_col),
        cell_y(cursor_row) + (FONT_HEIGHT - CURSOR_BAR_HEIGHT),
        FONT_WIDTH,
        CURSOR_BAR_HEIGHT,
        TERM_BG
    );
}

static void scroll_if_needed(void) {
    if (cursor_row < text_rows) {
        return;
    }

    gfx_scroll_up((uint32_t) FONT_HEIGHT, TERM_BG);
    cursor_row = text_rows - 1;
}

void terminal_initialize(void) {
    gfx_init();

    text_cols = (int) (gfx_screen_width() / FONT_WIDTH);
    text_rows = (int) (gfx_screen_height() / FONT_HEIGHT);

    gfx_clear(TERM_BG);

    cursor_row = 0;
    cursor_col = 0;
    cursor_blink_on = 0;
    last_blink_tick = 0;
}

void terminal_putchar(char c) {
    if (cursor_blink_on) {
        erase_cursor_bar();
    }

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = text_cols - 1;
        }
        clear_cell(cursor_col, cursor_row);
    } else {
        clear_cell(cursor_col, cursor_row);
        gfx_draw_char(cell_x(cursor_col), cell_y(cursor_row), c, TERM_FG);

        cursor_col++;
        if (cursor_col >= text_cols) {
            cursor_col = 0;
            cursor_row++;
            scroll_if_needed();
        }
    }

    cursor_blink_on = 1;
    draw_cursor_bar();
    last_blink_tick = timer_get_ticks();
}

void terminal_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_update_cursor(void) {
    uint32_t frequency = timer_get_frequency();
    if (frequency == 0) {
        return;
    }

    uint32_t blink_interval_ticks = (frequency * CURSOR_BLINK_MS) / 1000;
    if (blink_interval_ticks == 0) {
        blink_interval_ticks = 1;
    }

    uint32_t now = timer_get_ticks();
    if (now - last_blink_tick < blink_interval_ticks) {
        return;
    }

    last_blink_tick = now;
    cursor_blink_on = !cursor_blink_on;

    if (cursor_blink_on) {
        draw_cursor_bar();
    } else {
        erase_cursor_bar();
    }
}
