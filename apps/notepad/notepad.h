// apps/notepad/notepad.h

#ifndef NOTEPAD_H
#define NOTEPAD_H
#include <stdint.h>

void notepad_start(void);
void notepad_redraw(void);

void notepad_handle_char(char c);
void notepad_handle_click(int32_t x, int32_t y);

void notepad_update_cursor(void);

#endif
