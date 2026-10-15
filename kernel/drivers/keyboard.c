// kernel/drivers/keyboard.c

#include "keyboard.h"
#include "irq.h"
#include "io.h"

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

#define SC_LSHIFT_PRESS     0x2a
#define SC_RSHIFT_PRESS     0x36
#define SC_LSHIFT_RELEASE   0xaa
#define SC_RSHIFT_RELEASE   0xb6

#define SC_CAPSLOCK_PRESS   0x3a

#define SC_CTRL_PRESS       0x1d
#define SC_CTRL_RELEASE     0x9d

#define SC_EXT_UP           0x48
#define SC_EXT_DOWN         0x50
#define SC_EXT_LEFT         0x4b
#define SC_EXT_RIGHT        0x4d

#define SC_EXT_HOME         0x47
#define SC_EXT_END          0x4f

#define SC_EXT_DELETE       0x53

#define KEY_QUEUE_SIZE      64

static int shift_held = 0;
static int ctrl_held = 0;
static int caps_active = 0;
static int extended_prefix = 0;

static volatile char key_queue[KEY_QUEUE_SIZE];

static volatile int queue_head = 0;
static volatile int queue_tail = 0;

static void key_queue_push(char c) {
    int next = (queue_head + 1) % KEY_QUEUE_SIZE;
    if (next == queue_tail) return;

    key_queue[queue_head] = c;
    queue_head = next;
}

int keyboard_pop(char* out) {
    if (queue_tail == queue_head) return 0;

    *out = key_queue[queue_tail];
    queue_tail = (queue_tail + 1) % KEY_QUEUE_SIZE;
    return 1;
}

static char extended_scancode_to_key(unsigned char scancode) {
    switch (scancode) {
        case SC_EXT_UP:     return KEY_ARROW_UP;
        case SC_EXT_DOWN:   return KEY_ARROW_DOWN;
        case SC_EXT_LEFT:   return KEY_ARROW_LEFT;
        case SC_EXT_RIGHT:  return KEY_ARROW_RIGHT;

        case SC_EXT_HOME:   return KEY_HOME;
        case SC_EXT_END:    return KEY_END;
        
        case SC_EXT_DELETE: return KEY_DELETE;
        
        default:            return 0;
    }
}

static void keyboard_callback(struct registers regs) {
    (void) regs;

    unsigned char scancode = inb(0x60);

    if (scancode == 0xe0) {
        extended_prefix = -1;
        return;
    }

    if (extended_prefix) {
        extended_prefix = 0;

        if (scancode == SC_CTRL_PRESS) {
            ctrl_held = 1;
            return;
        }
        if (scancode == SC_CTRL_RELEASE) {
            ctrl_held = 0;
            return;
        }

        if (scancode & 0x80) {
            return;
        }

        char key = extended_scancode_to_key(scancode);
        if (key != 0) {
            key_queue_push(key);
        }
        return;
    }

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

    if (scancode == SC_CTRL_PRESS) {
        ctrl_held = 1;
        return;
    }
    if (scancode == SC_CTRL_RELEASE) {
        ctrl_held = 0;
        return;
    }

    if (scancode & 0x80) {
        return;
    }

    char lower = scancode_to_ascii[scancode];
    if (ctrl_held) {
        if (lower == 's') {
            key_queue_push(KEY_CTRL_S);
        }
        return;
    }

    int use_shifted = shift_held;

    if (caps_active && lower >= 'a' && lower <= 'z') {
        use_shifted = !use_shifted;
    }

    char c = use_shifted ? scancode_to_ascii_shifted[scancode] : lower;
    if (c != 0) {
        key_queue_push(c);
    }
}

void keyboard_install(void) {
    irq_install_handler(1, keyboard_callback);
}
