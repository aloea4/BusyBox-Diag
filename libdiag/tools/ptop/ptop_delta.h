#ifndef PTOP_DELTA_H
#define PTOP_DELTA_H

#include "ptop_snapshot.h"

typedef struct {
    pid_t pid;
    double cpu_percent;
    unsigned long rss_kb;
    char state;
    char comm[64];
    pid_t ppid;
} ptop_proc_view_t;

typedef struct {
    ptop_proc_view_t *items;
    size_t count;
} ptop_delta_result_t;

int ptop_delta_compute(
    const ptop_snapshot_t *prev,
    const ptop_snapshot_t *curr,
    ptop_delta_result_t *out);

void ptop_delta_free(ptop_delta_result_t *out);

#endif
