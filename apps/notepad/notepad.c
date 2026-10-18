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
#define NOTEPAD_DEFAULT_FILE "notes"

#define NOTEPAD_TOP         TITLE_BAR_HEIGHT
#define NOTEPAD_BOTTOM      TASKBAR_HEIGHT

#define STATUS_ROWS         1
#define STATUS_TEXT_X       8
#define STATUS_LINE_MAX     120

#define NOTE_FG             GFX_WHITE
#define NOTE_BG             GFX_BLACK
#define STATUS_BG           ((gfx_color_t){0x20, 0x20, 0x20})

#define CURSOR_BAR_HEIGHT   2
#define CURSOR_BLINK_MS     500

#define TAB_WIDTH           4

#define NOTEPAD_MAX_COLS    128
#define NOTEPAD_MAX_ROWS    48

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

static char file_path[FS_MAX_PATH];

static int dirty = 0;
static int save_allowed = 1;

static char incoming[NOTEPAD_MAX_BYTES];

static char shown[NOTEPAD_MAX_ROWS][NOTEPAD_MAX_COLS];
static char wanted[NOTEPAD_MAX_ROWS][NOTEPAD_MAX_COLS];

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
    if (text_cols > NOTEPAD_MAX_COLS) {
        text_cols = NOTEPAD_MAX_COLS;
    }
    if (text_rows > NOTEPAD_MAX_ROWS) {
        text_rows = NOTEPAD_MAX_ROWS;
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

static void find_cell_in(const char* buffer, int count, int* out_row, int* out_col) {
    int row = 0;
    int col = 0;

    for (int i = 0; i < count; i++) {
        advance(buffer[i], &row, &col);
    }

    *out_row = row;
    *out_col = col;
}

static void find_cell(int count, int* out_row, int* out_col) {
    find_cell_in(text, count, out_row, out_col);
}

static int last_text_row(void) {
    int row;
    int col;
    find_cell(length, &row, &col);
    return row;
}

static int buffer_fits_on_screen(const char* buffer, int count) {
    int row;
    int col;
    find_cell_in(buffer, count, &row, &col);
    return row < visible_rows();
}

static int text_fits_on_screen(void) {
    return buffer_fits_on_screen(text, length);
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

    append_text(line, &pos, file_path);
    append_text(line, &pos, "  |  Ctrl+S: save  |  ");

    kuitoa((uint32_t) length, number);
    append_text(line, &pos, number);
    append_text(line, &pos, " / ");

    kuitoa(NOTEPAD_MAX_BYTES, number);
    append_text(line, &pos, number);
    append_text(line, &pos, " bytes  |  ");

    append_text(line, &pos, status_note);

    gfx_draw_string(STATUS_TEXT_X, y, line, NOTE_FG);
}

static void draw_cell(int col, int row, char c) {
    gfx_draw_char_bg(cell_x(col), cell_y(row), c, NOTE_FG, NOTE_BG);
}

static void reset_shown(void) {
    for (int row = 0; row < NOTEPAD_MAX_ROWS; row++) {
        for (int col = 0; col < NOTEPAD_MAX_COLS; col++) {
            shown[row][col] = ' ';
        }
    }
}

static void build_wanted(void) {
    for (int row = 0; row < visible_rows(); row++) {
        for (int col = 0; col < text_cols; col++) {
            wanted[row][col] = ' ';
        }
    }

    int row = 0;
    int col = 0;

    for (int i = 0; i < length && row < visible_rows(); i++) {
        if (text[i] != '\n') {
            wanted[row][col] = text[i];
        }
        advance(text[i], &row, &col);
    }
}

static void refresh_text(void) {
    build_wanted();

    for (int row = 0; row < visible_rows(); row++) {
        for (int col = 0; col < text_cols; col++) {
            if (wanted[row][col] != shown[row][col]) {
                draw_cell(col, row, wanted[row][col]);
                shown[row][col] = wanted[row][col];
            }
        }
    }
}

static void paint_screen(void) {
    gfx_fill_rect(
        0, NOTEPAD_TOP,
        gfx_screen_width(), (uint32_t) text_rows * FONT_HEIGHT,
        NOTE_BG
    );

    reset_shown();
    refresh_text();
    draw_status();

    update_cursor_position();
    show_cursor();
}

// --- loading and saving ---

static void set_file_path(const char* path) {
    int i = 0;
    while (path[i] != '\0' && i < FS_MAX_PATH - 1) {
        file_path[i] = path[i];
        i++;
    }
    file_path[i] = '\0';
}

static int load_file(const char* path) {
    uint32_t size = 0;
    int result = fs_read_file(path, (uint8_t*) incoming, NOTEPAD_MAX_BYTES, &size);

    if (result == FS_ERR_NOT_FOUND) {
        length = 0;
        status_note = "new note";
        return 1;
    }
    if (result == FS_ERR_TOO_BIG) {
        status_note = "file too big for Notepad";
        return 0;
    }
    if (result == FS_ERR_IS_DIR) {
        status_note = "that is a folder, not a file";
        return 0;
    }
    if (result != FS_OK) {
        status_note = "cannot open that file";
        return 0;
    }
    if (!buffer_fits_on_screen(incoming, (int) size)) {
        status_note = "file too long to fit on screen";
        return 0;
    }

    for (uint32_t i = 0; i < size; i++) {
        text[i] = incoming[i];
    }
    length = (int) size;
    status_note = "loaded";
    return 1;
}

static void save_note(void) {
    if (!save_allowed) {
        status_note = "not saved - the file was never loaded";
        draw_status();
        return;
    }

    int result = fs_write_file(file_path, (const uint8_t*) text, (uint32_t) length);

    if (result == FS_OK) {
        status_note = "saved";
        dirty = 0;
    } else if (result == FS_ERR_FULL) {
        status_note = "save failed - file table full";
    } else {
        status_note = "save failed - bad path";
    }
    draw_status();
}

static void mark_dirty(void) {
    dirty = 1;
    status_note = "unsaved changes";
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

    text_insert(old_index, c);

    if (!text_fits_on_screen()) {
        text_remove(old_index);
        status_note = "screen is full";
        draw_status();
        show_cursor();
        return 0;
    }

    cursor_index++;

    refresh_text();

    update_cursor_position();
    desired_col = cursor_col;

    mark_dirty();
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

    cursor_index--;
    text_remove(cursor_index);
    update_cursor_position();
    desired_col = cursor_col;

    refresh_text();

    mark_dirty();
    draw_status();
    show_cursor();
}

static void delete_forward(void) {
    if (cursor_index >= length) {
        return;
    }

    hide_cursor();

    text_remove(cursor_index);
    refresh_text();
    desired_col = cursor_col;

    mark_dirty();
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
    set_file_path(NOTEPAD_DEFAULT_FILE);
    dirty = 0;

    save_allowed = load_file(file_path);
    if (!save_allowed) {
        length = 0;
    }

    cursor_index = length;
    paint_screen();
    desired_col = cursor_col;
}

void notepad_redraw(void) {
    compute_layout();
    paint_screen();
}

int notepad_open(const char* path) {
    if (dirty) {
        status_note = "unsaved changes - press Ctrl+S, then try again";
        draw_status();
        return 0;
    }

    if (!load_file(path)) {
        draw_status();
        return 0;
    }

    set_file_path(path);
    dirty = 0;
    save_allowed = 1;

    cursor_index = length;
    paint_screen();
    desired_col = cursor_col;
    return 1;
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

void notepad_handle_click(int32_t x, int32_t y) {
    int col = (int) (x / FONT_WIDTH);
    int row = (int) ((y - NOTEPAD_TOP) / FONT_HEIGHT);

    if (row < 0 || row >= visible_rows()) {
        return;
    }

    move_cursor(index_at(row, col), 0);
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
