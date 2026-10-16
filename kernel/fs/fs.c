// kernel/fs/fs.c

#include "fs.h"
#include "ata.h"
#include "kstring.h"

#define FS_TABLE_LBA      1
#define FS_TABLE_SECTORS  2
#define FS_TABLE_BYTES    (FS_TABLE_SECTORS * 512)
#define FS_DATA_START_LBA 100

#define FS_MAGIC 0x46535632
#define FS_ROOT 0xff

typedef struct {
    char name[FS_MAX_NAME_LEN];
    uint32_t start_lba;
    uint32_t size_bytes;
    uint8_t used;
    uint8_t is_dir;
    uint8_t parent;
} __attribute__((packed)) fs_entry_t;

typedef struct {
    uint32_t magic;
    fs_entry_t entries[FS_MAX_FILES];
} __attribute__((packed)) fs_table_t;

typedef char fs_table_must_fit[(sizeof(fs_table_t) <= FS_TABLE_BYTES) ? 1 : -1];

static fs_table_t table;
static int table_loaded = 0;

// --- small helpers ---

static void copy_bytes(void* dest, const void* src, uint32_t count) {
    uint8_t* d = (uint8_t*) dest;
    const uint8_t* s = (const uint8_t*) src;
    for (uint32_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
}

static void copy_name(char* dest, const char* src) {
    int i = 0;
    for (; i < FS_MAX_NAME_LEN - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int name_is_valid(const char* name) {
    uint32_t len = kstrlen(name);
    if (len == 0 || len >= FS_MAX_NAME_LEN) {
        return 0;
    }
    if (kstrcmp(name, ".") == 0 || kstrcmp(name, "..") == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < len; i++) {
        if (name[i] == '/') {
            return 0;
        }
    }
    return 1;
}

// --- loading and saving the table ---

static void fs_save_table(void) {
    uint8_t buffer[FS_TABLE_BYTES];
    for (uint32_t i = 0; i < FS_TABLE_BYTES; i++) {
        buffer[i] = 0;
    }
    copy_bytes(buffer, &table, sizeof(fs_table_t));

    for (uint32_t s = 0; s < FS_TABLE_SECTORS; s++) {
        ata_write_sector(FS_TABLE_LBA + s, buffer + s * 512);
    }
}

static void fs_load_table(void) {
    uint8_t buffer[FS_TABLE_BYTES];
    for (uint32_t s = 0; s < FS_TABLE_SECTORS; s++) {
        ata_read_sector(FS_TABLE_LBA + s, buffer + s * 512);
    }

    fs_table_t* on_disk = (fs_table_t*) buffer;
    if (on_disk->magic == FS_MAGIC) {
        copy_bytes(&table, buffer, sizeof(fs_table_t));
    } else {
        table.magic = FS_MAGIC;
        for (int i = 0; i < FS_MAX_FILES; i++) {
            table.entries[i].name[0] = '\0';
            table.entries[i].start_lba = 0;
            table.entries[i].size_bytes = 0;
            table.entries[i].used = 0;
            table.entries[i].is_dir = 0;
            table.entries[i].parent = FS_ROOT;
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

// --- looking things up ---

static int find_child(int parent, const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used) {
            continue;
        }
        if (table.entries[i].parent != parent) {
            continue;
        }
        if (kstrcmp(table.entries[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used) {
            return i;
        }
    }
    return -1;
}

static int has_children(int folder_index) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (table.entries[i].used && table.entries[i].parent == folder_index) {
            return 1;
        }
    }
    return 0;
}

// --- path walking ---

static int next_component(const char** cursor, char* out) {
    const char* p = *cursor;

    while (*p == '/') {
        p++;
    }
    if (*p == '\0') {
        *cursor = p;
        return 0;
    }

    int len = 0;
    while (*p != '/' && *p != '\0') {
        if (len >= FS_MAX_NAME_LEN - 1) {
            return -1;
        }
        out[len] = *p;
        len++;
        p++;
    }
    out[len] = '\0';

    *cursor = p;
    return 1;
}

static int resolve_parent(const char* path, int* parent_out, char* leaf_out) {
    const char* cursor = path;
    char part[FS_MAX_NAME_LEN];
    char next[FS_MAX_NAME_LEN];
    int parent = FS_ROOT;

    int status = next_component(&cursor, part);
    if (status <= 0) {
        return FS_ERR_BAD_PATH;
    }

    for (;;) {
        status = next_component(&cursor, next);
        if (status < 0) {
            return FS_ERR_BAD_PATH;
        }
        if (status == 0) {
            break;
        }

        int index = find_child(parent, part);
        if (index == -1) {
            return FS_ERR_NOT_FOUND;
        }
        if (!table.entries[index].is_dir) {
            return FS_ERR_NOT_DIR;
        }
        parent = index;
        copy_name(part, next);
    }

    copy_name(leaf_out, part);
    *parent_out = parent;
    return FS_OK;
}

static int resolve_entry(const char* path, int* index_out) {
    int parent;
    char leaf[FS_MAX_NAME_LEN];

    int result = resolve_parent(path, &parent, leaf);
    if (result != FS_OK) {
        return result;
    }

    int index = find_child(parent, leaf);
    if (index == -1) {
        return FS_ERR_NOT_FOUND;
    }

    *index_out = index;
    return FS_OK;
}

static int resolve_folder(const char* path, int* folder_out) {
    const char* cursor = path;
    char part[FS_MAX_NAME_LEN];

    if (next_component(&cursor, part) == 0) {
        *folder_out = FS_ROOT;
        return FS_OK;
    }

    int index;
    int result = resolve_entry(path, &index);
    if (result != FS_OK) {
        return result;
    }
    if (!table.entries[index].is_dir) {
        return FS_ERR_NOT_DIR;
    }

    *folder_out = index;
    return FS_OK;
}

// --- disk space ---

static uint32_t fs_sectors_needed(uint32_t bytes) {
    return (bytes + 511) / 512;
}

static uint32_t fs_find_free_space(uint32_t sectors_needed, int skip_index) {
    uint32_t range_start[FS_MAX_FILES];
    uint32_t range_end[FS_MAX_FILES];
    int range_count = 0;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!table.entries[i].used || table.entries[i].is_dir || i == skip_index) {
            continue;
        }
        range_start[range_count] = table.entries[i].start_lba;
        range_end[range_count] = table.entries[i].start_lba
            + fs_sectors_needed(table.entries[i].size_bytes);
        range_count++;
    }

    for (int i = 1; i < range_count; i++) {
        uint32_t s = range_start[i];
        uint32_t e = range_end[i];
        int j = i - 1;
        while (j >= 0 && range_start[j] > s) {
            range_start[j + 1] = range_start[j];
            range_end[j + 1] = range_end[j];
            j--;
        }
        range_start[j + 1] = s;
        range_end[j + 1] = e;
    }

    uint32_t cursor = FS_DATA_START_LBA;
    for (int i = 0; i < range_count; i++) {
        if (range_start[i] > cursor) {
            uint32_t gap = range_start[i] - cursor;
            if (gap >= sectors_needed) {
                return cursor;
            }
        }
        if (range_end[i] > cursor) {
            cursor = range_end[i];
        }
    }

    return cursor;
}

// --- public functions ---

int fs_write_file(const char* path, const uint8_t* data, uint32_t size) {
    fs_ensure_loaded();

    int parent;
    char leaf[FS_MAX_NAME_LEN];
    int result = resolve_parent(path, &parent, leaf);
    if (result != FS_OK) {
        return result;
    }
    if (!name_is_valid(leaf)) {
        return FS_ERR_BAD_PATH;
    }

    int existing_idx = find_child(parent, leaf);
    if (existing_idx != -1 && table.entries[existing_idx].is_dir) {
        return FS_ERR_IS_DIR;
    }

    int idx = existing_idx;
    if (idx == -1) {
        idx = find_free_slot();
        if (idx == -1) {
            return FS_ERR_FULL;
        }
    }

    uint32_t sectors_needed = fs_sectors_needed(size);
    uint32_t start = fs_find_free_space(sectors_needed, existing_idx);

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

    copy_name(table.entries[idx].name, leaf);
    table.entries[idx].start_lba = start;
    table.entries[idx].size_bytes = size;
    table.entries[idx].used = 1;
    table.entries[idx].is_dir = 0;
    table.entries[idx].parent = (uint8_t) parent;

    fs_save_table();
    return FS_OK;
}

int fs_read_file(const char* path, uint8_t* buffer, uint32_t buffer_size, uint32_t* out_size) {
    fs_ensure_loaded();

    int idx;
    int result = resolve_entry(path, &idx);
    if (result != FS_OK) {
        return result;
    }
    if (table.entries[idx].is_dir) {
        return FS_ERR_IS_DIR;
    }

    uint32_t size = table.entries[idx].size_bytes;
    if (size > buffer_size) {
        return FS_ERR_TOO_BIG;
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
    return FS_OK;
}

int fs_delete_file(const char* path) {
    fs_ensure_loaded();

    int idx;
    int result = resolve_entry(path, &idx);
    if (result != FS_OK) {
        return result;
    }

    if (table.entries[idx].is_dir && has_children(idx)) {
        return FS_ERR_NOT_EMPTY;
    }

    table.entries[idx].used = 0;
    fs_save_table();
    return FS_OK;
}

int fs_make_folder(const char* path) {
    fs_ensure_loaded();

    int parent;
    char leaf[FS_MAX_NAME_LEN];
    int result = resolve_parent(path, &parent, leaf);
    if (result != FS_OK) {
        return result;
    }
    if (!name_is_valid(leaf)) {
        return FS_ERR_BAD_PATH;
    }
    if (find_child(parent, leaf) != -1) {
        return FS_ERR_EXISTS;
    }

    int idx = find_free_slot();
    if (idx == -1) {
        return FS_ERR_FULL;
    }

    copy_name(table.entries[idx].name, leaf);
    table.entries[idx].start_lba = 0;
    table.entries[idx].size_bytes = 0;
    table.entries[idx].used = 1;
    table.entries[idx].is_dir = 1;
    table.entries[idx].parent = (uint8_t) parent;

    fs_save_table();
    return FS_OK;
}

int fs_rename(const char* path, const char* new_name) {
    fs_ensure_loaded();

    int idx;
    int result = resolve_entry(path, &idx);
    if (result != FS_OK) {
        return result;
    }
    if (!name_is_valid(new_name)) {
        return FS_ERR_BAD_PATH;
    }

    int clash = find_child(table.entries[idx].parent, new_name);
    if (clash != -1 && clash != idx) {
        return FS_ERR_EXISTS;
    }

    copy_name(table.entries[idx].name, new_name);
    fs_save_table();
    return FS_OK;
}

int fs_stat(const char* path, fs_info_t* out) {
    fs_ensure_loaded();

    int idx;
    int result = resolve_entry(path, &idx);
    if (result != FS_OK) {
        return result;
    }

    copy_name(out->name, table.entries[idx].name);
    out->size_bytes = table.entries[idx].size_bytes;
    out->is_dir = table.entries[idx].is_dir;
    return FS_OK;
}

int fs_list_dir(const char* path, fs_info_t out[FS_MAX_FILES]) {
    fs_ensure_loaded();

    int folder;
    int result = resolve_folder(path, &folder);
    if (result != FS_OK) {
        return result;
    }

    int count = 0;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (!table.entries[i].used || table.entries[i].parent != folder) {
                continue;
            }
            if (table.entries[i].is_dir != (pass == 0)) {
                continue;
            }
            copy_name(out[count].name, table.entries[i].name);
            out[count].size_bytes = table.entries[i].size_bytes;
            out[count].is_dir = table.entries[i].is_dir;
            count++;
        }
    }
    return count;
}
