// kernel/drivers/keyboard.h

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_ESCAPE      27

#define KEY_CTRL_S      0x13

#define KEY_ARROW_UP    0x01
#define KEY_ARROW_DOWN  0x02
#define KEY_ARROW_LEFT  0x03
#define KEY_ARROW_RIGHT 0x04

#define KEY_HOME        0x05
#define KEY_END         0x06

#define KEY_DELETE      0x7f

typedef void (*keyboard_handler_t)(char c);

void keyboard_install(void);
void keyboard_set_handler(keyboard_handler_t handler);

#endif
