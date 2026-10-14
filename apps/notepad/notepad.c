// apps/notepad/notepad.c

#include "notepad.h"
#include "title_bar.h"
#include "taskbar.h"
#include "graphics.h"
#include "font.h"
#include "kstring.h"
#include "fs.h"
#include "timer.h"
#include "keyboard.h"

// --- settings ---

#define NOTEPAD_MAX_BYTES   2048
#define NOTEPAD_FILE_NAME   "notes"

#define NOTEPAD_TOP         TITLE_BAR_HEIGHT
#define NOTEPAD_BOTTOM      TASKBAR_HEIGHT

#define STATUS_ROWS         1
#define STATUS_TEXT_X       8
#define STATUS_LINE_MAX     96

#define NOTE_FG             GFX_WHITE
#define NOTE_BG             GFX_BLACK
#define STATUS_BG           ((gfx_color_t){0x20, 0x20, 0x20})

#define CURSOR_BAR_HEIGHT   2
#define CURSOR_BLINK_MS     500

// --- state ---

static char text[NOTEPAD_MAX_BYTES];
static int length = 0;

static int text_cols;
static int text_rows;

static int cursor_row;
static int cursor_col;
static int cursor_blink_on;
static uint32_t last_blink_tick;

static const char* status_note = "";

// --- layout helpers & stuff ---

static void compute_layout(void) {
    uint32_t used = NOTEPAD_TOP + NOTEPAD_BOTTOM;
    uint32_t band_height = 0;
    if (gfx_screen_height() > used) {
        band_height = gfx_screen_height() - used;
    }

    text_cols = (int) (gfx_screen_width() / FONT_WIDTH);
    text_rows = (int) (band_height / FONT_HEIGHT);

    if (text_cols < 1) {
        text_cols = 1;
    }
    if (text_rows < STATUS_ROWS + 1) {
        text_rows = STATUS_ROWS + 1;
    }
}

static int visible_rows(void) {
    return text_rows - STATUS_ROWS;
}

static uint32_t cell_x(int col) {
    return (uint32_t) col * FONT_WIDTH;
}
static uint32_t cell_y(int row) {
    return NOTEPAD_TOP + (uint32_t) row * FONT_HEIGHT;
}

static void advance(char c, int* row, int* col) {
    if (c == '\n') {
        *col = 0;
        (*row)++;
        return;
    }

    (*col)++;
    if (*col >= text_cols) {
        *col = 0;
        (*row)++;
    }
}

static void find_cell(int count, int* out_row, int* out_col) {
    int row = 0;
    int col = 0;

    for (int i = 0; i < count; i++) {
        advance(text[i], &row, &col);
    }

    *out_row = row;
    *out_col = col;
}

static int cursor_fits(int count) {
    int row;
    int col;
    find_cell(count, &row, &col);
    return row < visible_rows();
}

// --- drawing ---

static void clear_cell(int col, int row) {
    gfx_fill_rect(cell_x(col), cell_y(row), FONT_WIDTH, FONT_HEIGHT, NOTE_BG);
}

static void paint_cursor_bar(gfx_color_t color) {
    gfx_fill_rect(
        cell_x(cursor_col),
        cell_y(cursor_row) + (FONT_HEIGHT - CURSOR_BAR_HEIGHT),
        FONT_WIDTH,
        CURSOR_BAR_HEIGHT,
        color
    );
}

static void show_cursor(void) {
    cursor_blink_on = 1;
    paint_cursor_bar(NOTE_FG);
    last_blink_tick = timer_get_ticks();
}

static void hide_cursor(void) {
    cursor_blink_on = 0;
    paint_cursor_bar(NOTE_BG);
}

static void append_text(char* line, int* pos, const char* piece) {
    for (int i = 0; piece[i] != '\0' && *pos < STATUS_LINE_MAX - 1; i++) {
        line[*pos] = piece[i];
        (*pos)++;
    }
    line[*pos] = '\0';
}

static void draw_status(void) {
    uint32_t y = cell_y(text_rows - STATUS_ROWS);
    gfx_fill_rect(0, y, gfx_screen_width(), FONT_HEIGHT, STATUS_BG);

    char line[STATUS_LINE_MAX];
    char number[12];
    int pos = 0;
    line[0] = '\0';

    append_text(line, &pos, "Ctrl+S: save  |  ");

    kuitoa((uint32_t) length, number);
    append_text(line, &pos, number);
    append_text(line, &pos, " / ");

    kuitoa(NOTEPAD_MAX_BYTES, number);
    append_text(line, &pos, number);
    append_text(line, &pos, " bytes  |  ");

    append_text(line, &pos, status_note);

    gfx_draw_string(STATUS_TEXT_X, y, line, NOTE_FG);
}

static void draw_all_text(void) {
    int row = 0;
    int col = 0;

    for (int i = 0; i < length && row < visible_rows(); i++) {
        if (text[i] != '\n') {
            gfx_draw_char(cell_x(col), cell_y(row), text[i], NOTE_FG);
        }
        advance(text[i], &row, &col);
    }
}

static void paint_screen(void) {
    gfx_fill_rect(0, NOTEPAD_TOP, gfx_screen_width(), (uint32_t) text_rows * FONT_HEIGHT, NOTE_BG);

    draw_all_text();
    draw_status();

    find_cell(length, &cursor_row, &cursor_col);
    show_cursor();
}

// --- save/load things ---

static void load_note(void) {
    uint32_t size = 0;
    int result = fs_read_file(NOTEPAD_FILE_NAME, (uint8_t*) text, NOTEPAD_MAX_BYTES, &size);

    if (result == 0) {
        length = (int) size;
        status_note = "loaded";
    } else if (result == -2) {
        length = 0;
        status_note = "saved note too big to load";
    } else {
        length = 0;
        status_note = "new note";
    }
}

static void trim_to_screen(void) {
    while (length > 0 && !cursor_fits(length)) {
        length--;
    }
}

static void save_note(void) {
    int result = fs_write_file(NOTEPAD_FILE_NAME, (const uint8_t*) text, (uint32_t) length);

    if (result == 0) {
        status_note = "saved";
    } else {
        status_note = "save failed - file table full";
    }
    draw_status();
}

// --- editing ---

static void insert_char(char c) {
    if (length >= NOTEPAD_MAX_BYTES) {
        status_note = "note is full";
        draw_status();
        return;
    }

    text[length] = c;
    length++;

    if (!cursor_fits(length)) {
        length--;
        status_note = "screen is full";
        draw_status();
        return;
    }

    hide_cursor();

    if (c != '\n') {
        gfx_draw_char(cell_x(cursor_col), cell_y(cursor_row), c, NOTE_FG);
    }

    find_cell(length, &cursor_row, &cursor_col);

    status_note = "unsaved changes";
    draw_status();
    show_cursor();
}

static void delete_last_char(void) {
    if (length == 0) {
        return;
    }

    hide_cursor();
    length--;

    find_cell(length, &cursor_row, &cursor_col);
    clear_cell(cursor_col, cursor_row);

    status_note = "unsaved changes";
    draw_status();
    show_cursor();
}

// --- pub funcs (the app_t slots) ---

void notepad_start(void) {
    compute_layout();
    load_note();
    trim_to_screen();
    paint_screen();
}

void notepad_redraw(void) {
    compute_layout();
    paint_screen();
}

void notepad_handle_char(char c) {
    if (c == KEY_CTRL_S) {
        save_note();
        return;
    }

    if (c == '\b') {
        delete_last_char();
        return;
    }

    if (c == '\n' || (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR)) {
        insert_char(c);
    }
}

void notepad_update_cursor(void) {
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

    if (cursor_blink_on) {
        hide_cursor();
    } else {
        show_cursor();
    }
    last_blink_tick = now;
}
