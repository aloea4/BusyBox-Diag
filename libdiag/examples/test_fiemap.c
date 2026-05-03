#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *path = "Makefile";
    diag_fiemap_info_t info;
    int ret;

    if (argc >= 2) {
        path = argv[1];
    }

    ret = diag_fs_read_fiemap(path, &info);
    if (ret == DIAG_ERR_UNSUPPORTED) {
        printf("FIEMAP unsupported for %s: %s\n", path, diag_strerror(ret));
        printf("test_fiemap: PASS (unsupported handled)\n");
        return 0;
    }

    if (ret != DIAG_OK) {
        printf("diag_fs_read_fiemap failed for %s: %s\n", path, diag_strerror(ret));
        return 1;
    }

    printf("FIEMAP snapshot:\n");
    printf("  path           = %s\n", path);
    printf("  extent_count   = %u\n", info.extent_count);
    printf("  logical_bytes  = %llu\n", info.logical_bytes);
    printf("  physical_bytes = %llu\n", info.physical_bytes);

    printf("test_fiemap: PASS\n");
    return 0;
}