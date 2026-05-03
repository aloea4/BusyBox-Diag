#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>

static unsigned long calc_used_percent(const diag_fs_info_t *fs)
{
    if (fs == NULL || fs->total_kb == 0) {
        return 0;
    }

    return (fs->used_kb * 100UL) / fs->total_kb;
}

static unsigned long calc_inode_used_percent(const diag_fs_info_t *fs)
{
    unsigned long used;

    if (fs == NULL || fs->files_total == 0) {
        return 0;
    }

    if (fs->files_total < fs->files_free) {
        return 0;
    }

    used = fs->files_total - fs->files_free;
    return (used * 100UL) / fs->files_total;
}

int main(int argc, char **argv)
{
    const char *path = "/";
    diag_fs_info_t fs;
    int ret;

    if (argc >= 2) {
        path = argv[1];
    }

    ret = diag_fs_read(path, &fs);
    if (ret != DIAG_OK) {
        printf("diag_fs_read failed for %s: %s\n", path, diag_strerror(ret));
        return 1;
    }

    printf("Filesystem snapshot:\n");
    printf("  path        = %s\n", fs.path);
    printf("  fs_type     = 0x%lx (%s)\n", fs.fs_type, diag_fs_type_name(fs.fs_type));
    printf("  total_kb    = %lu\n", fs.total_kb);
    printf("  free_kb     = %lu\n", fs.free_kb);
    printf("  avail_kb    = %lu\n", fs.avail_kb);
    printf("  used_kb     = %lu\n", fs.used_kb);
    printf("  used%%       = %lu\n", calc_used_percent(&fs));
    printf("  files_total = %lu\n", fs.files_total);
    printf("  files_free  = %lu\n", fs.files_free);
    printf("  inode_used%% = %lu\n", calc_inode_used_percent(&fs));

    if (fs.total_kb == 0 && fs.files_total == 0) {
        printf("note: zero-sized filesystem, likely pseudo filesystem\n");
    }

    printf("test_fs: PASS\n");
    return 0;
}