// boot/vigil/memory.c

#include "memory.h"
#include "ata.h"

#define VIGIL_STATE_LBA 0
#define VIGIL_MAGIC 0x56494731

static vigil_state_t current_state;

void vigil_memory_load(vigil_state_t* state) {
    uint8_t sector[512];
    ata_read_sector(VIGIL_STATE_LBA, sector);

    vigil_state_t* on_disk = (vigil_state_t*) sector;
    if (on_disk->magic == VIGIL_MAGIC) {
        *state = *on_disk;
    } else {
        state->magic = VIGIL_MAGIC;
        state->boot_count = 0;
    }
}

void vigil_memory_save(const vigil_state_t* state) {
    uint8_t sector[512] = {0};
    vigil_state_t* on_disk = (vigil_state_t*) sector;
    *on_disk = *state;
    ata_write_sector(VIGIL_STATE_LBA, sector);

    current_state = *state;
}

uint32_t vigil_memory_get_boot_count(void) {
    return current_state.boot_count;
}
