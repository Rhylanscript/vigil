// boot/drivers/mouse.h

#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>

void mouse_install(void);

void mouse_update_cursor(void);

int32_t mouse_get_x(void);
int32_t mouse_get_y(void);

int mouse_left_button(void);
int mouse_right_button(void);
int mouse_middle_button(void);

#endif
