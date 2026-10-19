// kernel/vigil/app_format.h

// What a loadable VIGIL app file ('vapp') looks like, and where in ram
// apps are allowed to live

#ifndef VIGIL_APP_FORMAT_H
#define VIGIL_APP_FORMAT_H
#include <stdint.h>

// the first 4 bytes of every app file spell "VAPP" in ascii
// (V=0x56 A=0x41 P=0x50 P=0x50)
#define VAPP_MAGIC           0x50504156
#define VAPP_FORMAT_VERSION  1

// --- RAM slots ---

// each loaded app gets a fixed size block of RAM (a "slot") an app is
// built for ONE specific slot address, so no address patching is needed
// The heap is 0x200000-0x2fffff, so slots start just above that
//   slot 0: 0x400000   slot 1: 0x440000   slot 2: 0x480000   slot 3: 0x4c0000
//   slot 4: 0x500000   slot 5: 0x540000   slot 6: 0x580000   slot 7: 0x5c0000
#define LOADER_SLOTS_BASE    0x400000
#define LOADER_SLOT_SIZE     0x40000     // 256 KB per app
#define LOADER_SLOT_COUNT    8

// the 24 bytes at the very start of an app file, the linker writes this
// (see apps/sdk/app.ld); apps never fill it in themselves
typedef struct {
    uint32_t magic;            // must be VAPP_MAGIC
    uint32_t format_version;   // must be VAPP_FORMAT_VERSION
    uint32_t load_address;     // the address the app was built to run at
    uint32_t file_size;        // bytes in the file (header included)
    uint32_t memory_size;      // bytes needed in RAM: file + zeroed .bss area
    uint32_t entry_offset;     // distance from the start to app_entry()
} app_header_t;

#endif
