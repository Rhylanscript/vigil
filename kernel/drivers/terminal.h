// kernel/drivers/terminal.h

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

/// Clears the screen and resets cursor position to top left. Call this 
/// once before printing anything.
void terminal_initialize(void);

/// Prints a single character at the current cursor position then
/// advances the cursor, handling newlines and scrolling automatically
void terminal_putchar(char c);

/// Prints a null terminated string one char at a time via
/// `terminal_putchar`
void terminal_print(const char* c);

void terminal_update_cursor(void);

void terminal_set_top_margin(uint32_t pixels);
void terminal_set_bottom_margin(uint32_t pixels);

void terminal_redraw(void);

#endif
