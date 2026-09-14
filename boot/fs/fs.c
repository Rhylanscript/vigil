// boot/fs/fs.c

#include "fs.h"
#include "ata.h"

#define FS_TABLE_LBA 1
#define FS_DATA_START_LBA 100
#define FS_MAGIC 0x46535631

typedef struct {
    char name[FS_MAX_NAME_LEN];
    uint32_t start_lba;
    uint32_t size_bytes;
    uint8_t used;
} __attribute__((packed)) fs_entry_t;

typedef struct {
    uint32_t magic;
    fs_entry_t entries[FS_MAX_FILES];
} __attribute__((packed)) fs_table_t;

static fs_table_t table;
static int table_loaded = 0;

static void fs_save_table(void) {
    uint8_t sector[512] = {0};
    fs_table_t* on_disk = (fs_table_t*) sector;
    *on_disk = table;
    ata_write_sector(FS_TABLE_LBA, sector);
}

static void fs_load_table(void) {
    uint8_t sector[512];
    ata_read_sector(FS_TABLE_LBA, sector);

    fs_table_t* on_disk = (fs_table_t*) sector;
    if (on_disk->magic == FS_MAGIC) {
        table = *on_disk;
    } else {
        table.magic = FS_MAGIC;
        for (int i = 0; i < FS_MAX_FILES; i++) {
            table.entries[i].used = 0;
        }
        fs_save_table();
    }
    table_loaded = 1;
}

static void fs_ensure_loaded(void) {
    if (!table_loaded) {
        fs_load_table();
    }
}

static int fs_find_entry(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used) {
            continue;
        }
        int match = 1;
        for (int j = 0; j < FS_MAX_NAME_LEN; j++) {
            if (table.entries[i].name[j] != name[j]) {
                match = 0;
                break;
            }
            if (name[j] == '\0') {
                break;
            }
        }
        if (match) {
            return i;
        }
    }
    return -1;
}

static uint32_t fs_next_free_lba(void) {
    uint32_t highest = FS_DATA_START_LBA;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used) {
            continue;
        }
        uint32_t sectors = (table.entries[i].size_bytes + 511) / 512;
        uint32_t end = table.entries[i].start_lba + sectors;
        if (end > highest) {
            highest = end;
        }
    }
    return highest;
}

int fs_write_file(const char* name, const uint8_t* data, uint32_t size) {
    fs_ensure_loaded();

    int idx = fs_find_entry(name);
    if (idx == -1) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (!table.entries[i].used) {
                idx = i;
                break;
            }
        }
    }
    if (idx == -1) {
        return -1;
    }

    uint32_t start = fs_next_free_lba();
    uint32_t sectors_needed = (size + 511) / 512;

    for (uint32_t s = 0; s < sectors_needed; s++) {
        uint8_t sector[512] = {0};
        uint32_t offset = s * 512;
        uint32_t remaining = size - offset;
        uint32_t chunk = (remaining < 512) ? remaining : 512;
        for (uint32_t b = 0; b < chunk; b++) {
            sector[b] = data[offset + b];
        }
        ata_write_sector(start + s, sector);
    }

    int i = 0;
    for (; i < FS_MAX_NAME_LEN - 1 && name[i] != '\0'; i++) {
        table.entries[idx].name[i] = name[i];
    }
    table.entries[idx].name[i] = '\0';
    table.entries[idx].start_lba = start;
    table.entries[idx].size_bytes = size;
    table.entries[idx].used = 1;

    fs_save_table();
    return 0;
}

int fs_read_file(const char* name, uint8_t* buffer, uint32_t buffer_size, uint32_t* out_size) {
    fs_ensure_loaded();

    int idx = fs_find_entry(name);
    if (idx == -1) {
        return -1;
    }

    uint32_t size = table.entries[idx].size_bytes;
    if (size > buffer_size) {
        return -2;
    }

    uint32_t sectors = (size + 511) / 512;
    for (uint32_t s = 0; s < sectors; s++) {
        uint8_t sector[512];
        ata_read_sector(table.entries[idx].start_lba + s, sector);

        uint32_t offset = s * 512;
        uint32_t remaining = size - offset;
        uint32_t chunk = (remaining < 512) ? remaining : 512;
        for (uint32_t b = 0; b < chunk; b++) {
            buffer[offset + b] = sector[b];
        }
    }

    *out_size = size;
    return 0;
}

int fs_delete_file(const char* name) {
    fs_ensure_loaded();

    int idx = fs_find_entry(name);
    if (idx == -1) {
        return -1;
    }

    table.entries[idx].used = 0;
    fs_save_table();
    return 0;
}

int fs_list_files(char names_out[FS_MAX_FILES][FS_MAX_NAME_LEN], uint32_t sizes_out[FS_MAX_FILES]) {
    fs_ensure_loaded();

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used) {
            continue;
        }
        for (int j = 0; j < FS_MAX_NAME_LEN; j++) {
            names_out[count][j] = table.entries[i].name[j];
        }
        sizes_out[count] = table.entries[i].size_bytes;
        count++;
    }
    return count;
}
