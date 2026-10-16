// kernel/ui/menu.h

#ifndef MENU_H
#define MENU_H
#include <stdint.h>

#define MENU_MAX_ITEMS 8
#define MENU_NO_PICK (-1)

void menu_open(int32_t x, int32_t y, const char* const* labels, int count);

int menu_is_open(void);

int menu_click(int32_t px, int32_t py);

void menu_close(void);

#endif
