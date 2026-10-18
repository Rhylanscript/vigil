// kernel/fs/fs.h

#ifndef FS_H
#define FS_H
#include <stdint.h>

#define FS_MAX_NAME_LEN 16
#define FS_MAX_FILES 32
#define FS_MAX_PATH 64

// --- return codes ---

#define FS_OK             0
#define FS_ERR_NOT_FOUND -1
#define FS_ERR_TOO_BIG   -2
#define FS_ERR_NOT_EMPTY -3
#define FS_ERR_EXISTS    -4
#define FS_ERR_BAD_PATH  -5
#define FS_ERR_NOT_DIR   -6
#define FS_ERR_IS_DIR    -7
#define FS_ERR_FULL      -8

typedef struct {
    char name[FS_MAX_NAME_LEN];
    uint32_t size_bytes;
    uint8_t is_dir;
} fs_info_t;

int fs_write_file(const char* path, const uint8_t* data, uint32_t size);
int fs_read_file(const char* path, uint8_t* buffer, uint32_t buffer_size, uint32_t* out_size);

int fs_delete_file(const char* path);

int fs_make_folder(const char* path);

int fs_rename(const char* path, const char* new_name);

int fs_stat(const char* path, fs_info_t* out);

int fs_list_dir(const char* path, fs_info_t out[FS_MAX_FILES]);

#endif
