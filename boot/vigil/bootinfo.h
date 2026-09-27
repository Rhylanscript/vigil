// boot/vigil/bootinfo.h

#ifndef BOOT_INFO_H
#define BOOT_INFO_H
#include <stdint.h>

typedef struct {
    uint32_t framebuffer_addr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
} __attribute__((packed)) boot_info_t;

#define BOOT_INFO_ADDR 0x0600

static inline boot_info_t* boot_info(void) {
    return (boot_info_t*) BOOT_INFO_ADDR;
}

#endif
