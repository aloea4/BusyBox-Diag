#include "libdiag/diag_fs.h"
#include "libdiag/diag_common.h"

#include <stdio.h>
#include <string.h>
#include <sys/vfs.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef TMPFS_MAGIC
#define TMPFS_MAGIC 0x01021994
#endif

#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC 0x9fa0
#endif

#ifndef SYSFS_MAGIC
#define SYSFS_MAGIC 0x62656572
#endif

#ifndef EXT_SUPER_MAGIC
#define EXT_SUPER_MAGIC 0xef53
#endif

#ifndef BTRFS_SUPER_MAGIC
#define BTRFS_SUPER_MAGIC 0x9123683e
#endif

#ifndef XFS_SUPER_MAGIC
#define XFS_SUPER_MAGIC 0x58465342
#endif

#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC 0x6969
#endif

int diag_fs_read(const char *path, diag_fs_info_t *out)
{
    struct statfs st;
    unsigned long block_size;
    unsigned long total_blocks;
    unsigned long free_blocks;
    unsigned long avail_blocks;

    if (path == NULL || out == NULL || path[0] == '\0') {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    if (statfs(path, &st) != 0) {
        return DIAG_ERR_IO;
    }

    snprintf(out->path, sizeof(out->path), "%s", path);

    block_size = (unsigned long)st.f_bsize;
    total_blocks = (unsigned long)st.f_blocks;
    free_blocks = (unsigned long)st.f_bfree;
    avail_blocks = (unsigned long)st.f_bavail;

    out->total_kb = (total_blocks * block_size) / 1024UL;
    out->free_kb = (free_blocks * block_size) / 1024UL;
    out->avail_kb = (avail_blocks * block_size) / 1024UL;

    if (out->total_kb >= out->free_kb) {
        out->used_kb = out->total_kb - out->free_kb;
    } else {
        out->used_kb = 0;
    }

    out->files_total = (unsigned long)st.f_files;
    out->files_free = (unsigned long)st.f_ffree;
    out->fs_type = (unsigned long)st.f_type;

    return DIAG_OK;
}

int diag_fs_read_fiemap(const char *path, diag_fiemap_info_t *out)
{
    int fd;
    int ret;
    size_t fiemap_size;
    struct fiemap *fiemap;
    unsigned int max_extents = 128;

    if (path == NULL || out == NULL || path[0] == '\0') {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return DIAG_ERR_IO;
    }

    fiemap_size = sizeof(struct fiemap) +
                  max_extents * sizeof(struct fiemap_extent);

    fiemap = (struct fiemap *)calloc(1, fiemap_size);
    if (fiemap == NULL) {
        close(fd);
        return DIAG_ERR_IO;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0ULL;
    fiemap->fm_flags = FIEMAP_FLAG_SYNC;
    fiemap->fm_extent_count = max_extents;

    ret = ioctl(fd, FS_IOC_FIEMAP, fiemap);
    if (ret < 0) {
        int saved_errno = errno;

        free(fiemap);
        close(fd);

        if (saved_errno == EOPNOTSUPP ||
            saved_errno == ENOTTY ||
            saved_errno == EINVAL) {
            return DIAG_ERR_UNSUPPORTED;
        }

        return DIAG_ERR_IO;
    }

    out->extent_count = fiemap->fm_mapped_extents;

    for (unsigned int i = 0; i < fiemap->fm_mapped_extents; i++) {
        out->logical_bytes += fiemap->fm_extents[i].fe_length;
        out->physical_bytes += fiemap->fm_extents[i].fe_length;
    }

    free(fiemap);
    close(fd);

    return DIAG_OK;
}

const char *diag_fs_type_name(unsigned long fs_type)
{
    switch (fs_type) {
    case TMPFS_MAGIC:
        return "tmpfs";
    case PROC_SUPER_MAGIC:
        return "proc";
    case SYSFS_MAGIC:
        return "sysfs";
    case EXT_SUPER_MAGIC:
        return "ext";
    case BTRFS_SUPER_MAGIC:
        return "btrfs";
    case XFS_SUPER_MAGIC:
        return "xfs";
    case NFS_SUPER_MAGIC:
        return "nfs";
    default:
        return "unknown";
    }
}