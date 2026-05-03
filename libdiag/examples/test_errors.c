#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"
#include "libdiag/diag_fs.h"
#include "libdiag/diag_net.h"
#include "libdiag/diag_parse.h"

#include <stdio.h>

static int expect_error(const char *name, int got, int expected)
{
    if (got != expected) {
        printf("%s failed: expected %s, got %s\n",
               name,
               diag_strerror(expected),
               diag_strerror(got));
        return 1;
    }

    printf("%s: PASS (%s)\n", name, diag_strerror(got));
    return 0;
}

static int test_invalid_args(void)
{
    int fail = 0;

    fail += expect_error(
        "diag_proc_read invalid pid",
        diag_proc_read(-1, NULL),
        DIAG_ERR_INVALID_ARG
    );

    fail += expect_error(
        "diag_fs_read null",
        diag_fs_read(NULL, NULL),
        DIAG_ERR_INVALID_ARG
    );

    fail += expect_error(
        "diag_fs_read_fiemap null",
        diag_fs_read_fiemap(NULL, NULL),
        DIAG_ERR_INVALID_ARG
    );

    fail += expect_error(
        "diag_net_foreach_tcp null callback",
        diag_net_foreach_tcp(NULL, NULL),
        DIAG_ERR_INVALID_ARG
    );

    fail += expect_error(
        "diag_parse_ulong null",
        diag_parse_ulong(NULL, NULL),
        DIAG_ERR_INVALID_ARG
    );

    return fail;
}

static int test_io_errors(void)
{
    int fail = 0;
    diag_fs_info_t fs;
    diag_fiemap_info_t fiemap;
    diag_proc_info_t proc;

    fail += expect_error(
        "diag_fs_read not_exist",
        diag_fs_read("/not_exist_libdiag_test", &fs),
        DIAG_ERR_IO
    );

    fail += expect_error(
        "diag_fs_read_fiemap not_exist",
        diag_fs_read_fiemap("/not_exist_libdiag_test", &fiemap),
        DIAG_ERR_IO
    );

    fail += expect_error(
        "diag_proc_read not_exist_pid",
        diag_proc_read(99999999, &proc),
        DIAG_ERR_IO
    );

    return fail;
}

static int test_unsupported(void)
{
    int ret;
    diag_fiemap_info_t fiemap;

    ret = diag_fs_read_fiemap("/proc/cpuinfo", &fiemap);

    if (ret != DIAG_ERR_UNSUPPORTED && ret != DIAG_ERR_IO) {
        printf("diag_fs_read_fiemap /proc/cpuinfo failed: unexpected %s\n",
               diag_strerror(ret));
        return 1;
    }

    printf("diag_fs_read_fiemap /proc/cpuinfo: PASS (%s)\n",
           diag_strerror(ret));
    return 0;
}

static int test_pseudo_fs(void)
{
    int ret;
    diag_fs_info_t fs;

    ret = diag_fs_read("/proc", &fs);
    if (ret != DIAG_OK) {
        printf("diag_fs_read /proc failed: %s\n", diag_strerror(ret));
        return 1;
    }

    printf("diag_fs_read /proc: PASS type=%s total_kb=%lu files_total=%lu\n",
           diag_fs_type_name(fs.fs_type),
           fs.total_kb,
           fs.files_total);

    return 0;
}

int main(void)
{
    int fail = 0;

    fail += test_invalid_args();
    fail += test_io_errors();
    fail += test_unsupported();
    fail += test_pseudo_fs();

    if (fail != 0) {
        printf("test_errors: FAIL\n");
        return 1;
    }

    printf("test_errors: PASS\n");
    return 0;
}