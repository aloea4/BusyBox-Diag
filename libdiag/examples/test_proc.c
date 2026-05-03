#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"

#include <stdio.h>

static int print_proc(const diag_proc_info_t *p, void *user)
{
    int *count = (int *)user;

    printf(
        "%5d %5d %c %8lu %8lu %8lu %s\n",
        p->pid,
        p->ppid,
        p->state,
        p->rss_kb,
        p->utime,
        p->stime,
        p->comm
    );

    (*count)++;

    if (*count >= 20) {
        return 1;
    }

    return 0;
}

int main(void)
{
    int count = 0;
    int ret;

    printf("  PID  PPID S   RSS_KB    UTIME    STIME COMM\n");

    ret = diag_proc_foreach(print_proc, &count);

    /*
     * callback return 1 means intentional stop after 20 rows.
     */
    if (ret != DIAG_OK && ret != 1) {
        printf("diag_proc_foreach failed: %s\n", diag_strerror(ret));
        return 1;
    }

    if (count == 0) {
        printf("no process found\n");
        return 1;
    }

    printf("test_proc: PASS (%d processes shown)\n", count);
    return 0;
}