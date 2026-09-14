// boot/drivers/ata.c

#include "ata.h"
#include "io.h"

#define ATA_DATA        0x1f0
#define ATA_ERROR       0x1f1
#define ATA_SECCOUNT    0x1f2
#define ATA_LBA_LOW     0x1f3
#define ATA_LBA_MID     0x1f4
#define ATA_LBA_HIGH    0x1f5
#define ATA_DRIVE_HEAD  0x1f6
#define ATA_STATUS      0x1f7
#define ATA_COMMAND     0x1f7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_DRQ  0x08

static void ata_wait_ready(void) {
    while (inb(ATA_STATUS) & ATA_STATUS_BUSY) {

    }
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ)) {

    }
}

static void ata_select_sector(uint32_t lba) {
    outb(ATA_DRIVE_HEAD, 0xe0 | ((lba >> 24) & 0x0f));

    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW, (uint8_t) (lba & 0xff));
    outb(ATA_LBA_MID, (uint8_t) ((lba >> 8) & 0xff));
    outb(ATA_LBA_HIGH, (uint8_t) ((lba >> 16) & 0xff));
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_ready();
    ata_select_sector(lba);
    outb(ATA_COMMAND, ATA_CMD_READ);

    ata_wait_ready();
    ata_wait_drq();

    uint16_t* buf16 = (int16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_DATA);
    }
}

void ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    ata_wait_ready();
    ata_select_sector(lba);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    ata_wait_ready();
    ata_wait_drq();

    const uint16_t* buf16 = (const uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, buf16[i]);
    }

    outb(ATA_COMMAND, 0xe7);
    ata_wait_ready();
}
