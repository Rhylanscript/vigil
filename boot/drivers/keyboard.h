// boot/keyboard.h

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_ARROW_UP 0x01
#define KEY_ARROW_DOWN 0x02

typedef void (*keyboard_handler_t)(char c);

void keyboard_install(void);
void keyboard_set_handler(keyboard_handler_t handler);

#endif
