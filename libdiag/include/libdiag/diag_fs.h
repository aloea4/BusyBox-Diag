#ifndef LIBDIAG_DIAG_FS_H
#define LIBDIAG_DIAG_FS_H

#include "libdiag/diag_common.h"

typedef struct {
    char path[DIAG_PATH_MAX];

    unsigned long total_kb;
    unsigned long free_kb;
    unsigned long avail_kb;
    unsigned long used_kb;

    unsigned long files_total;
    unsigned long files_free;

    unsigned long fs_type;
} diag_fs_info_t;

typedef struct {
    unsigned int extent_count;
    unsigned long long logical_bytes;
    unsigned long long physical_bytes;
} diag_fiemap_info_t;

int diag_fs_read(const char *path, diag_fs_info_t *out);
int diag_fs_read_fiemap(const char *path, diag_fiemap_info_t *out);
const char *diag_fs_type_name(unsigned long fs_type);

#endif