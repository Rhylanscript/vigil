// apps/tetris/tetris.c

#include "tetris.h"

// --- pieces ---

static const int8_t piece_size[TETRIS_PIECE_KINDS] = { 4, 2, 3, 3, 3, 3, 3 };

static const int8_t piece_cells[TETRIS_PIECE_KINDS][4][2] = {
    { {0, 1}, {1, 1}, {2, 1}, {3, 1} }, // I
    { {0, 0}, {1, 0}, {0, 1}, {1, 1} }, // O
    { {1, 0}, {0, 1}, {1, 1}, {2, 1} }, // T
    { {1, 0}, {2, 0}, {0, 1}, {1, 1} }, // S
    { {0, 0}, {1, 0}, {1, 1}, {2, 1} }, // Z
    { {0, 0}, {0, 1}, {1, 1}, {2, 1} }, // J
    { {2, 0}, {0, 1}, {1, 1}, {2, 1} }, // L
};

static const uint32_t line_points[5] = { 0, 100, 300, 500, 800 };

// --- state ---

static uint8_t board[TETRIS_ROWS][TETRIS_COLS];

static int current_kind = 0;
static int current_rotation = 0;
static int current_x = 0;
static int current_y = 0;
static int ghost_y = 0;

static int next_kind = 0;
static int game_over = 0;

static uint32_t score = 0;
static uint32_t lines = 0;

static uint32_t rng_state = 1;

// --- random numbers ---

static uint32_t next_random(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int random_piece(void) {
    return (int) (next_random() % TETRIS_PIECE_KINDS);
}

// --- piece geometry ---

static void piece_cell(int kind, int rotation, int index, int* out_x, int* out_y) {
    int size = piece_size[kind];
    int x = piece_cells[kind][index][0];
    int y = piece_cells[kind][index][1];

    for (int turn = 0; turn < rotation; turn++) {
        int new_x = size - 1 - y;
        int new_y = x;
        x = new_x;
        y = new_y;
    }

    *out_x = x;
    *out_y = y;
}

static int fits(int kind, int rotation, int piece_x, int piece_y) {
    for (int i = 0; i < 4; i++) {
        int cell_x;
        int cell_y;
        piece_cell(kind, rotation, i, &cell_x, &cell_y);

        int col = piece_x + cell_x;
        int row = piece_y + cell_y;

        if (col < 0 || col >= TETRIS_COLS || row < 0 || row >= TETRIS_ROWS) {
            return 0;
        }
        if (board[row][col] != 0) {
            return 0;
        }
    }
    return 1;
}

static void update_ghost(void) {
    ghost_y = current_y;
    while (fits(current_kind, current_rotation, current_x, ghost_y + 1)) {
        ghost_y++;
    }
}

// --- piece landing ---

static void spawn_piece(void) {
    current_kind = next_kind;
    next_kind = random_piece();
    current_rotation = 0;
    current_x = (TETRIS_COLS - piece_size[current_kind]) / 2;
    current_y = 0;

    if (!fits(current_kind, current_rotation, current_x, current_y)) {
        game_over = 1;
    }
    update_ghost();
}

static void lock_piece(void) {
    for (int i = 0; i < 4; i++) {
        int cell_x;
        int cell_y;
        piece_cell(current_kind, current_rotation, i, &cell_x, &cell_y);
        board[current_y + cell_y][current_x + cell_x] = (uint8_t) (current_kind + 1);
    }
}

static int clear_full_lines(void) {
    int cleared = 0;
    int row = TETRIS_ROWS - 1;

    while (row >= 0) {
        int full = 1;
        for (int col = 0; col < TETRIS_COLS; col++) {
            if (board[row][col] == 0) {
                full = 0;
                break;
            }
        }

        if (!full) {
            row--;
            continue;
        }

        for (int above = row; above > 0; above--) {
            for (int col = 0; col < TETRIS_COLS; col++) {
                board[above][col] = board[above - 1][col];
            }
        }
        for (int col = 0; col < TETRIS_COLS; col++) {
            board[0][col] = 0;
        }
        cleared++;
    }

    return cleared;
}

// --- public funcs ---

void tetris_start(uint32_t seed) {
    for (int row = 0; row < TETRIS_ROWS; row++) {
        for (int col = 0; col < TETRIS_COLS; col++) {
            board[row][col] = 0;
        }
    }

    rng_state = (seed == 0) ? 1 : seed;
    score = 0;
    lines = 0;
    game_over = 0;

    next_kind = random_piece();
    spawn_piece();
}

int tetris_move(int dx) {
    if (game_over) {
        return 0;
    }
    if (!fits(current_kind, current_rotation, current_x + dx, current_y)) {
        return 0;
    }
    current_x += dx;
    update_ghost();
    return 1;
}

int tetris_rotate(void) {
    if (game_over) {
        return 0;
    }

    static const int kicks[5] = { 0, -1, 1, -2, 2 };
    int new_rotation = (current_rotation + 1) % 4;

    for (int i = 0; i < 5; i++) {
        if (fits(current_kind, new_rotation, current_x + kicks[i], current_y)) {
            current_rotation = new_rotation;
            current_x += kicks[i];
            update_ghost();
            return 1;
        }
    }
    return 0;
}

int tetris_step_down(void) {
    if (game_over) {
        return 0;
    }

    if (fits(current_kind, current_rotation, current_x, current_y + 1)) {
        current_y++;
        return 1;
    }

    lock_piece();
    int cleared = clear_full_lines();
    score += line_points[cleared] * tetris_level();
    lines += (uint32_t) cleared;
    spawn_piece();
    return 0;
}

void tetris_hard_drop(void) {
    while (tetris_step_down()) {
    }
}

int tetris_cell_at(int col, int row) {
    if (col < 0 || col >= TETRIS_COLS || row < 0 || row >= TETRIS_ROWS) {
        return 0;
    }
    if (board[row][col] != 0) {
        return board[row][col];
    }

    for (int i = 0; i < 4; i++) {
        int cell_x;
        int cell_y;
        piece_cell(current_kind, current_rotation, i, &cell_x, &cell_y);
        if (current_x + cell_x == col && current_y + cell_y == row) {
            return current_kind + 1;
        }
    }
    if (!game_over && ghost_y > current_y) {
        for (int i = 0; i < 4; i++) {
            int cell_x;
            int cell_y;
            piece_cell(current_kind, current_rotation, i, &cell_x, &cell_y);
            if (current_x + cell_x == col && ghost_y + cell_y == row) {
                return (current_kind + 1) | TETRIS_GHOST_FLAG;
            }
        }
    }
    return 0;
}

int tetris_preview_cell_at(int col, int row) {
    for (int i = 0; i < 4; i++) {
        int cell_x;
        int cell_y;
        piece_cell(next_kind, 0, i, &cell_x, &cell_y);
        if (cell_x == col && cell_y == row) {
            return next_kind + 1;
        }
    }
    return 0;
}

int tetris_is_over(void)    { return game_over; }
uint32_t tetris_score(void) { return score; }
uint32_t tetris_lines(void) { return lines; }
uint32_t tetris_level(void) { return lines / 10 + 1; }
