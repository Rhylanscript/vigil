// boot/lib/kprintf.h

#ifndef KPRINTF_H
#define KPRINTF_H

// printf style formatted out writing directly to terminal
// supports:
//   %d - signed decimal int
//   %x - unsigned hexadecimal int (lowercase, no 0x prefix)
//   %s - null terminated string
//   %c - single character
//   %% - '%' symbol
void kprintf(const char* format, ...);

#endif
