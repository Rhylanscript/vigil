// boot/lib/kprintf.c

#include "kprintf.h"
#include "kstring.h"
#include "terminal.h"
#include <stdarg.h>

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[12];

    for (int i = 0; format[i] != '\0'; i++) {
        char c = format[i];

        if (c != '%') {
            terminal_putchar(c);
            continue;
        }

        i++;
        char specifier = format[i];

        switch (specifier) {
            case 'd': {
                int value = va_arg(args, int);
                kitoa(value, buffer);
                terminal_print(buffer);
                break;
            }
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                khtoa((uint32_t) value, buffer);
                terminal_print(buffer);
                break;
            }
            case 's': {
                char* value = va_arg(args, char*);
                terminal_print(value);
                break;
            }
            case 'c': {
                char value = (char) va_arg(args, int);
                terminal_putchar(value);
                break;
            }
            case '%': {
                terminal_putchar('%');
                break;
            }
            case '\0': {
                i--;
                break;
            }
            default: {
                terminal_putchar('%');
                terminal_putchar(specifier);
                break;
            }
        }
    }

    va_end(args);
}
