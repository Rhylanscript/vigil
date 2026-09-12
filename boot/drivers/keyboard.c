// boot/drivers/keyboard.c

#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "terminal.h"

static const char scancode_to_ascii[128] = {
    0, 27,
    '1','2','3','4','5','6','7','8','9','0','-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' '
};

static const char scancode_to_ascii_shifted[128] = {
    0, 27,
    '!','@','#','$','%','^','&','*','(',')','_','+',
    '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}',
    '\n',
    0,
    'A','S','D','F','G','H','J','K','L',':','"','~',
    0,
    '|','Z','X','C','V','B','N','M','<','>','?',
    0,
    '*',
    0,
    ' '
};

#define SC_LSHIFT_PRESS   0x2a
#define SC_RSHIFT_PRESS   0x36
#define SC_LSHIFT_RELEASE 0xaa
#define SC_RSHIFT_RELEASE 0xb6
#define SC_CAPSLOCK_PRESS 0x3a

static int shift_held = 0;
static int caps_active = 0;

static void keyboard_callback(struct registers regs) {
    (void) regs;

    unsigned char scancode = inb(0x60);

    if (scancode == SC_LSHIFT_PRESS || scancode == SC_RSHIFT_PRESS) {
        shift_held = 1;
        return;
    }
    if (scancode == SC_LSHIFT_RELEASE || scancode == SC_RSHIFT_RELEASE) {
        shift_held = 0;
        return;
    }
    if (scancode == SC_CAPSLOCK_PRESS) {
        caps_active = !caps_active;
        return;
    }

    if (scancode & 0x80) {
        return;
    }

    char lower = scancode_to_ascii[scancode];
    int use_shifted = shift_held;

    if (caps_active && lower >= 'a' && lower <= 'z') {
        use_shifted = !use_shifted;
    }

    char c = use_shifted ? scancode_to_ascii_shifted[scancode] : lower;
    if (c != 0) {
        terminal_putchar(c);
    }
}

void keyboard_install(void) {
    irq_install_handler(1, keyboard_callback);
}
