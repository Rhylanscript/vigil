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

#define TAB_WIDTH           4

// --- state ---

static char text[NOTEPAD_MAX_BYTES];
static int length = 0;

static int text_cols;
static int text_rows;

static int cursor_row;
static int cursor_col;
static int cursor_index = 0;

static int desired_col = 0;

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

static int last_text_row(void) {
    int row;
    int col;
    find_cell(length, &row, &col);
    return row;
}

static int text_fits_on_screen(void) {
    return last_text_row() < visible_rows();
}

static int index_at(int target_row, int target_col) {
    int row = 0;
    int col = 0;
    int last_on_row = length;

    for (int i = 0; i <= length; i++) {
        if (row == target_row) {
            if (col >= target_col) {
                return i;
            }
            last_on_row = i;
        } else if (row > target_row) {
            break;
        }

        if (i < length) {
            advance(text[i], &row, &col);
        }
    }

    return last_on_row;
}

static void update_cursor_position(void) {
    find_cell(cursor_index, &cursor_row, &cursor_col);
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

    if (cursor_index < length && text[cursor_index] != '\n') {
        gfx_draw_char(cell_x(cursor_col), cell_y(cursor_row), text[cursor_index], NOTE_FG);
    }
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

static void draw_text_from_row(int first_row) {
    int row = 0;
    int col = 0;

    for (int i = 0; i < length && row < visible_rows(); i++) {
        if (row >= first_row && text[i] != '\n') {
            gfx_draw_char(cell_x(col), cell_y(row), text[i], NOTE_FG);
        }
        advance(text[i], &row, &col);
    }
}

static void repaint_from(int index) {
    int first_row;
    int col;
    find_cell(index, &first_row, &col);

    if (first_row >= visible_rows()) {
        return;
    }

    gfx_fill_rect(
        0, cell_y(first_row),
        gfx_screen_width(), (uint32_t) (visible_rows() - first_row) * FONT_HEIGHT,
        NOTE_BG
    );
    draw_text_from_row(first_row);
}

static void paint_screen(void) {
    gfx_fill_rect(
        0, NOTEPAD_TOP,
        gfx_screen_width(), (uint32_t) text_rows * FONT_HEIGHT,
        NOTE_BG
    );

    draw_text_from_row(0);
    draw_status();

    update_cursor_position();
    show_cursor();
}

// --- loading and saving ---

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
    while (length > 0 && !text_fits_on_screen()) {
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

// --- editing the text array ---

static void text_insert(int index, char c) {
    for (int i = length; i > index; i--) {
        text[i] = text[i - 1];
    }
    text[index] = c;
    length++;
}

static void text_remove(int index) {
    for (int i = index; i < length - 1; i++) {
        text[i] = text[i + 1];
    }
    length--;
}

// --- editing actions ---

static int insert_char(char c) {
    if (length >= NOTEPAD_MAX_BYTES) {
        status_note = "note is full";
        draw_status();
        return 0;
    }

    hide_cursor();

    int old_index = cursor_index;
    int appending = (cursor_index == length);

    text_insert(old_index, c);

    if (!text_fits_on_screen()) {
        text_remove(old_index);
        status_note = "screen is full";
        draw_status();
        show_cursor();
        return 0;
    }

    cursor_index++;

    if (appending) {
        if (c != '\n') {
            gfx_draw_char(cell_x(cursor_col), cell_y(cursor_row), c, NOTE_FG);
        }
    } else {
        repaint_from(old_index);
    }

    update_cursor_position();
    desired_col = cursor_col;

    status_note = "unsaved changes";
    draw_status();
    show_cursor();
    return 1;
}

static void insert_tab(void) {
    int spaces = TAB_WIDTH - (cursor_col % TAB_WIDTH);

    for (int i = 0; i < spaces; i++) {
        if (!insert_char(' ')) {
            return;
        }
    }
}

static void backspace(void) {
    if (cursor_index == 0) {
        return;
    }

    hide_cursor();

    int at_end = (cursor_index == length);

    cursor_index--;
    text_remove(cursor_index);
    update_cursor_position();
    desired_col = cursor_col;

    if (at_end) {
        clear_cell(cursor_col, cursor_row);
    } else {
        repaint_from(cursor_index);
    }

    status_note = "unsaved changes";
    draw_status();
    show_cursor();
}

static void delete_forward(void) {
    if (cursor_index >= length) {
        return;
    }

    hide_cursor();

    text_remove(cursor_index);
    repaint_from(cursor_index);
    desired_col = cursor_col;

    status_note = "unsaved changes";
    draw_status();
    show_cursor();
}

// --- moving the cursor ---

static void move_cursor(int new_index, int keep_column) {
    hide_cursor();

    cursor_index = new_index;
    update_cursor_position();

    if (!keep_column) {
        desired_col = cursor_col;
    }

    show_cursor();
}

static void move_left(void) {
    if (cursor_index > 0) {
        move_cursor(cursor_index - 1, 0);
    }
}

static void move_right(void) {
    if (cursor_index < length) {
        move_cursor(cursor_index + 1, 0);
    }
}

static void move_home(void) {
    move_cursor(index_at(cursor_row, 0), 0);
}

static void move_end(void) {
    move_cursor(index_at(cursor_row, text_cols), 0);
}

static void move_up(void) {
    if (cursor_row > 0) {
        move_cursor(index_at(cursor_row - 1, desired_col), 1);
    }
}

static void move_down(void) {
    if (cursor_row < last_text_row()) {
        move_cursor(index_at(cursor_row + 1, desired_col), 1);
    }
}

// --- pub funcs (the app_t slots) ---

void notepad_start(void) {
    compute_layout();
    load_note();
    trim_to_screen();

    cursor_index = length;
    paint_screen();
    desired_col = cursor_col;
}

void notepad_redraw(void) {
    compute_layout();
    paint_screen();
}

void notepad_handle_char(char c) {
    if (c == KEY_CTRL_S) {
        save_note();
    } else if (c == '\b') {
        backspace();
    } else if (c == KEY_DELETE) {
        delete_forward();
    } else if (c == '\t') {
        insert_tab();
    } else if (c == KEY_ARROW_LEFT) {
        move_left();
    } else if (c == KEY_ARROW_RIGHT) {
        move_right();
    } else if (c == KEY_ARROW_UP) {
        move_up();
    } else if (c == KEY_ARROW_DOWN) {
        move_down();
    } else if (c == KEY_HOME) {
        move_home();
    } else if (c == KEY_END) {
        move_end();
    } else if (c == '\n' || (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR)) {
        insert_char(c);
    }

    // ignore any other input
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
