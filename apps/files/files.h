// apps/files/files.h

#ifndef FILES_H
#define FILES_H
#include <stdint.h>

void files_start(void);
void files_redraw(void);
void files_handle_char(char c);
void files_handle_click(int32_t x, int32_t y);
void files_handle_right_click(int32_t x, int32_t y);
void files_handle_menu_pick(int index);

#endif