#include "ptop_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    diag_proc_info_t *buf;
    size_t count;
    size_t cap;
} proc_buf_t;

static int collect_cb(const diag_proc_info_t *info, void *user)
{
    proc_buf_t *b = user;

    if (b->count >= b->cap) {
        size_t new_cap = (b->cap == 0) ? 256 : b->cap * 2;
        void *p = realloc(b->buf, new_cap * sizeof(diag_proc_info_t));
        if (!p) return -1;
        b->buf = p;
        b->cap = new_cap;
    }

    b->buf[b->count++] = *info;
    return 0;
}

static int collect_meminfo(ptop_snapshot_t *snap)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1;

    char key[64];
    unsigned long value = 0;
    char unit[16];

    while (fscanf(fp, "%63[^:]: %lu %15s\n", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal") == 0)
            snap->mem_total_kb = value;
        else if (strcmp(key, "MemFree") == 0)
            snap->mem_free_kb = value;
        else if (strcmp(key, "Buffers") == 0)
            snap->buffers_kb = value;
        else if (strcmp(key, "Cached") == 0)
            snap->cached_kb = value;
        else if (strcmp(key, "SwapTotal") == 0)
            snap->swap_total_kb = value;
        else if (strcmp(key, "SwapFree") == 0)
            snap->swap_free_kb = value;
    }

    fclose(fp);
    return 0;
}

int ptop_snapshot_collect(ptop_snapshot_t *snap)
{
    memset(snap, 0, sizeof(*snap));

    clock_gettime(CLOCK_MONOTONIC, &snap->ts);

    if (diag_cpu_read_snapshot(&snap->cpu) < 0)
        return -1;

    if (collect_meminfo(snap) < 0)
        return -1;

    proc_buf_t b = {0};

    if (diag_proc_foreach(collect_cb, &b) < 0) {
        free(b.buf);
        return -1;
    }

    snap->procs = b.buf;
    snap->proc_count = b.count;

    return 0;
}

void ptop_snapshot_free(ptop_snapshot_t *snap)
{
    free(snap->procs);
    snap->procs = NULL;
    snap->proc_count = 0;
}
