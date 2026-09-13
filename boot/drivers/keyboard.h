// boot/keyboard.h

#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef void (*keyboard_handler_t)(char c);

void keyboard_install(void);
void keyboard_set_handler(keyboard_handler_t handler);

#endif
