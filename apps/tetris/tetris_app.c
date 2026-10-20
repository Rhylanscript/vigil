// apps/tetris/tetris_app.c

#include "app.h"
#include "api.h"
#include "font.h"
#include "keyboard.h"
#include "title_bar.h"
#include "taskbar.h"
#include "tetris.h"

// --- api pointer ---

static const vigil_api_t* api = 0;

// --- layout (pixels) ---

#define CELL_SIZE       28
#define BOARD_W         (TETRIS_COLS * CELL_SIZE)
#define BOARD_H         (TETRIS_ROWS * CELL_SIZE)

#define PANEL_GAP       32
#define PANEL_W         220
#define TOTAL_W         (BOARD_W + PANEL_GAP + PANEL_W)

#define PREVIEW_PX      (TETRIS_PREVIEW_SIZE * CELL_SIZE)

#define NEXT_LABEL_Y    0
#define PREVIEW_Y       24
#define SCORE_LABEL_Y   160
#define LEVEL_LABEL_Y   216
#define LINES_LABEL_Y   272
#define BEST_LABEL_Y    328
#define VALUE_OFFSET    20
#define STATUS_Y        384
#define STATUS_H        56
#define CONTROLS_Y      448
#define CONTROLS_LINE_H 18

// --- colours ---

#define LABEL_COLOR     ((gfx_color_t){0x90, 0x90, 0x90})
#define GRID_COLOR      ((gfx_color_t){0x24, 0x24, 0x24})
#define GOLD_COLOR      ((gfx_color_t){0xff, 0xd0, 0x40})

static const gfx_color_t piece_colors[TETRIS_PIECE_KINDS + 1] = {
    { 0x0a, 0x0a, 0x0a },   // 0 = empty square
    { 0x00, 0xc8, 0xe0 },   // 1 = I (cyan)
    { 0xe8, 0xd8, 0x20 },   // 2 = O (yellow)
    { 0xa0, 0x40, 0xd0 },   // 3 = T (purple)
    { 0x30, 0xc0, 0x40 },   // 4 = S (green)
    { 0xd8, 0x30, 0x30 },   // 5 = Z (red)
    { 0x30, 0x50, 0xe0 },   // 6 = J (blue)
    { 0xf0, 0x90, 0x20 },   // 7 = L (orange)
};

// --- speed ---

#define DROP_START_MS   800
#define DROP_STEP_MS    70
#define DROP_MIN_MS     100

// --- saved high score ---

#define BEST_FILE "tetris_best"

// --- state ---

static uint32_t origin_x = 0;
static uint32_t origin_y = 0;
static uint32_t panel_x = 0;

static uint8_t shown[TETRIS_ROWS][TETRIS_COLS];

static uint32_t last_drop = 0;
static int paused = 0;

static uint32_t best_score = 0;
static int over_handled = 0;
static int new_best = 0;
static int save_failed = 0;

// --- saving and loading highscore ---

static void load_best(void) {
    char text[16];
    uint32_t size = 0;

    best_score = 0;

    int result = api->fs_read_file(BEST_FILE, (uint8_t*) text, sizeof(text) - 1, &size);
    if (result != FS_OK) {
        return;
    }

    for (uint32_t i = 0; i < size && i < 9; i++) {
        if (text[i] < '0' || text[i] > '9') {
            break;
        }
        best_score = best_score * 10 + (uint32_t) (text[i] - '0');
    }
}

static void save_best(void) {
    char text[12];
    api->kuitoa(best_score, text);

    int result = api->fs_write_file(BEST_FILE, (const uint8_t*) text, api->kstrlen(text));
    save_failed = (result != FS_OK);
}

static void check_game_over(void) {
    if (!tetris_is_over() || over_handled) {
        return;
    }
    over_handled = 1;

    if (tetris_score() > best_score) {
        best_score = tetris_score();
        new_best = 1;
        save_best();
    }
}

// --- layout ---

static uint32_t band_height(void) {
    uint32_t used = TITLE_BAR_HEIGHT + TASKBAR_HEIGHT;
    if (api->gfx_screen_height() > used) {
        return api->gfx_screen_height() - used;
    }
    return 0;
}

static void compute_layout(void) {
    origin_x = 0;
    if (api->gfx_screen_width() > TOTAL_W) {
        origin_x = (api->gfx_screen_width() - TOTAL_W) / 2;
    }

    origin_y = TITLE_BAR_HEIGHT;
    if (band_height() > BOARD_H) {
        origin_y += (band_height() - BOARD_H) / 2;
    }

    panel_x = origin_x + BOARD_W + PANEL_GAP;
}

// --- drawing ---

static void draw_cell(uint32_t x, uint32_t y, int kind_number) {
    api->gfx_fill_rect(x, y, CELL_SIZE, CELL_SIZE, GRID_COLOR);
    api->gfx_fill_rect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, piece_colors[kind_number]);
}

static void refresh_board(void) {
    for (int row = 0; row < TETRIS_ROWS; row++) {
        for (int col = 0; col < TETRIS_COLS; col++) {
            int value = tetris_cell_at(col, row);
            if (value == shown[row][col]) {
                continue;
            }

            draw_cell(
                origin_x + (uint32_t) col * CELL_SIZE,
                origin_y + (uint32_t) row * CELL_SIZE,
                value
            );
            shown[row][col] = (uint8_t) value;
        }
    }
}

static void draw_preview(void) {
    uint32_t box_x = panel_x;
    uint32_t box_y = origin_y + PREVIEW_Y;

    api->gfx_fill_rect(box_x, box_y, PREVIEW_PX, PREVIEW_PX, GFX_BLACK);

    for (int row = 0; row < TETRIS_PREVIEW_SIZE; row++) {
        for (int col = 0; col < TETRIS_PREVIEW_SIZE; col++) {
            int value = tetris_preview_cell_at(col, row);
            if (value != 0) {
                draw_cell(box_x + (uint32_t) col * CELL_SIZE, box_y + (uint32_t) row * CELL_SIZE, value);
            }
        }
    }
}

static void draw_number(uint32_t label_y, uint32_t value) {
    char number[12];
    uint32_t y = origin_y + label_y + VALUE_OFFSET;

    api->gfx_fill_rect(panel_x, y, PANEL_W, FONT_HEIGHT, GFX_BLACK);
    api->kuitoa(value, number);
    api->gfx_draw_string(panel_x, y, number, GFX_WHITE);
}

static void draw_stats(void) {
    draw_number(SCORE_LABEL_Y, tetris_score());
    draw_number(LEVEL_LABEL_Y, tetris_level());
    draw_number(LINES_LABEL_Y, tetris_lines());
    draw_number(BEST_LABEL_Y, best_score);
}

static void draw_status(void) {
    uint32_t y = origin_y + STATUS_Y;
    api->gfx_fill_rect(panel_x, y, PANEL_W, STATUS_H, GFX_BLACK);

    if (tetris_is_over()) {
        api->gfx_draw_string(panel_x, y, "GAME OVER", GFX_RED);

        if (new_best) {
            api->gfx_draw_string(panel_x, y + 20, "New best score!", GOLD_COLOR);
        }
        if (save_failed) {
            api->gfx_draw_string(panel_x, y + 38, "Could not save score", GFX_RED);
        } else {
            api->gfx_draw_string(panel_x, y + 38, "Press R to play again", GFX_WHITE);
        }
    } else if (paused) {
        api->gfx_draw_string(panel_x, y, "PAUSED", GFX_GREEN);
        api->gfx_draw_string(panel_x, y + 20, "Press P to resume", GFX_WHITE);
    }
}

static void draw_side_panel(void) {
    draw_preview();
    draw_stats();
    draw_status();
}

static void draw_labels(void) {
    api->gfx_draw_string(panel_x, origin_y + NEXT_LABEL_Y, "NEXT", LABEL_COLOR);
    api->gfx_draw_string(panel_x, origin_y + SCORE_LABEL_Y, "SCORE", LABEL_COLOR);
    api->gfx_draw_string(panel_x, origin_y + LEVEL_LABEL_Y, "LEVEL", LABEL_COLOR);
    api->gfx_draw_string(panel_x, origin_y + LINES_LABEL_Y, "LINES", LABEL_COLOR);
    api->gfx_draw_string(panel_x, origin_y + BEST_LABEL_Y, "BEST", LABEL_COLOR);

    static const char* const controls[] = {
        "Left/Right: move",
        "Up: rotate",
        "Down: soft drop",
        "Space: hard drop",
        "P: pause",
        "R: restart",
        "Esc: minimize",
    };
    int count = (int) (sizeof(controls) / sizeof(controls[0]));

    for (int i = 0; i < count; i++) {
        api->gfx_draw_string(
            panel_x,
            origin_y + CONTROLS_Y + (uint32_t) i * CONTROLS_LINE_H,
            controls[i],
            LABEL_COLOR
        );
    }
}

static void paint(void) {
    api->gfx_fill_rect(0, TITLE_BAR_HEIGHT, api->gfx_screen_width(), band_height(), GFX_BLACK);
    api->gfx_draw_rect_outline(origin_x - 2, origin_y - 2, BOARD_W + 4, BOARD_H + 4, GFX_WHITE);

    for (int row = 0; row < TETRIS_ROWS; row++) {
        for (int col = 0; col < TETRIS_COLS; col++) {
            shown[row][col] = 0xff;
        }
    }

    refresh_board();
    draw_labels();
    draw_side_panel();
}

// --- game flow ---

static uint32_t drop_interval_ticks(void) {
    uint32_t speedup = (tetris_level() - 1) * DROP_STEP_MS;
    uint32_t ms = DROP_MIN_MS;
    if (speedup < DROP_START_MS - DROP_MIN_MS) {
        ms = DROP_START_MS - speedup;
    }

    uint32_t ticks = (api->timer_get_frequency() * ms) / 1000;
    return (ticks == 0) ? 1 : ticks;
}

static void new_game(void) {
    uint32_t now = api->timer_get_ticks();

    tetris_start(now * 2654435761u + 1);

    paused = 0;
    over_handled = 0;
    new_best = 0;
    save_failed = 0;
    last_drop = now;

    paint();
}

static void after_step(int moved) {
    api->mouse_hide_cursor();
    refresh_board();

    if (!moved) {
        check_game_over();
        draw_side_panel();
    }
}

static void after_move(int changed) {
    if (changed) {
        api->mouse_hide_cursor();
        refresh_board();
    }
}

// --- app_t slots ---

static void tetris_app_start(void) {
    compute_layout();
    load_best();
    new_game();
}

static void tetris_app_redraw(void) {
    compute_layout();
    last_drop = api->timer_get_ticks();
    paint();
}

static void tetris_app_key(char c) {
    // R works any time, even after game over
    if (c == 'r' || c == 'R') {
        api->mouse_hide_cursor();
        new_game();
        return;
    }

    if (tetris_is_over()) {
        return;
    }

    if (c == 'p' || c == 'P') {
        paused = !paused;
        last_drop = api->timer_get_ticks();
        api->mouse_hide_cursor();
        draw_status();
        return;
    }

    if (paused) {
        return;
    }

    if (c == KEY_ARROW_LEFT) {
        after_move(tetris_move(-1));
    } else if (c == KEY_ARROW_RIGHT) {
        after_move(tetris_move(1));
    } else if (c == KEY_ARROW_UP) {
        after_move(tetris_rotate());
    } else if (c == KEY_ARROW_DOWN) {
        last_drop = api->timer_get_ticks();
        after_step(tetris_step_down());
    } else if (c == ' ') {
        tetris_hard_drop();
        last_drop = api->timer_get_ticks();
        after_step(0);
    }
}

static void tetris_app_tick(void) {
    if (paused || tetris_is_over()) {
        return;
    }

    uint32_t now = api->timer_get_ticks();
    if (now - last_drop < drop_interval_ticks()) {
        return;
    }

    last_drop = now;
    after_step(tetris_step_down());
}

// --- entry point ---

int app_entry(const vigil_api_t* kernel_api, app_t* out_app) {
    if (kernel_api->version != VIGIL_API_VERSION || kernel_api->struct_size < sizeof(vigil_api_t)) {
        return 1;
    }
    api = kernel_api;

    out_app->name = "Tetris";
    out_app->start = tetris_app_start;
    out_app->redraw = tetris_app_redraw;
    out_app->on_key = tetris_app_key;
    out_app->on_tick = tetris_app_tick;
    return 0;
}
