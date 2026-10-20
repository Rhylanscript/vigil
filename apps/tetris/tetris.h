// apps/tetris/tetris.h

#ifndef TETRIS_H
#define TETRIS_H
#include <stdint.h>

#define TETRIS_COLS          10
#define TETRIS_ROWS          20
#define TETRIS_PIECE_KINDS   7  // I, O, T, S, Z, J, L
#define TETRIS_PREVIEW_SIZE  4

// --- game control ---

void tetris_start(uint32_t seed);
int tetris_move(int dx);

int tetris_rotate(void);
int tetris_step_down(void);

void tetris_hard_drop(void);

// --- reading the game ---

int tetris_cell_at(int col, int row);

int tetris_preview_cell_at(int col, int row);

int      tetris_is_over(void);
uint32_t tetris_score(void);
uint32_t tetris_lines(void);
uint32_t tetris_level(void);

#endif
