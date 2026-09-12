// boot/terminal.c

#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((unsigned short*) 0xb8000)
#define WHITE_ON_BLACK 0x0f

static int cursor_row;
static int cursor_col;

static unsigned short make_cell(char c, unsigned char colour) {
    return (unsigned short) c | ((unsigned short) colour << 8);
}

void terminal_initialize(void) {
    volatile unsigned short* video = VGA_MEMORY;

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i] = make_cell(' ', WHITE_ON_BLACK);
    }

    cursor_row = 0;
    cursor_col = 0;
}

static void scroll_if_needed(void) {
    if (cursor_row < VGA_HEIGHT) {
        return;
    }

    volatile unsigned short* video = VGA_MEMORY;

    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            video[(row - 1) * VGA_WIDTH + col] = video[row * VGA_WIDTH + col];
        }
    }

    for (int col = 0; col < VGA_WIDTH; col++) {
        video[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = make_cell(' ', WHITE_ON_BLACK);
    }

    cursor_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    volatile unsigned short* video = VGA_MEMORY;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
        return;
    }

    video[cursor_row * VGA_WIDTH + cursor_col] = make_cell(c, WHITE_ON_BLACK);

    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
    }
}

void terminal_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}
