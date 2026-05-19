#ifndef PTOP_SNAPSHOT_H
#define PTOP_SNAPSHOT_H

#include <time.h>
#include "libdiag/diag_proc.h"

typedef struct {
    diag_proc_info_t *procs;
    size_t proc_count;

    diag_cpu_snapshot_t cpu;
    unsigned long mem_total_kb;
    unsigned long mem_free_kb;
    unsigned long buffers_kb;
    unsigned long cached_kb;
    unsigned long swap_total_kb;
    unsigned long swap_free_kb;

    struct timespec ts;
} ptop_snapshot_t;

int ptop_snapshot_collect(ptop_snapshot_t *snap);
void ptop_snapshot_free(ptop_snapshot_t *snap);

#endif
