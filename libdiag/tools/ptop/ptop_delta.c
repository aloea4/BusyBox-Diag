#include "ptop_delta.h"
#include <stdlib.h>
#include <string.h>

static int cmp_proc_pid(const void *a, const void *b)
{
    const diag_proc_info_t *pa = a;
    const diag_proc_info_t *pb = b;
    if (pa->pid < pb->pid) return -1;
    if (pa->pid > pb->pid) return 1;
    return 0;
}

static const diag_proc_info_t *find_prev_proc(
    const diag_proc_info_t *prev_list, size_t prev_count, pid_t pid)
{
    diag_proc_info_t key;
    memset(&key, 0, sizeof(key));
    key.pid = pid;

    return bsearch(&key, prev_list, prev_count,
                   sizeof(diag_proc_info_t), cmp_proc_pid);
}

int ptop_delta_compute(
    const ptop_snapshot_t *prev,
    const ptop_snapshot_t *curr,
    ptop_delta_result_t *out)
{
    memset(out, 0, sizeof(*out));

    if (!prev || !curr)
        return -1;

    // 複製 prev procs 出來排序（避免破壞 snapshot）
    diag_proc_info_t *prev_sorted =
        malloc(prev->proc_count * sizeof(diag_proc_info_t));

    if (!prev_sorted)
        return -1;

    memcpy(prev_sorted, prev->procs,
           prev->proc_count * sizeof(diag_proc_info_t));

    qsort(prev_sorted, prev->proc_count,
          sizeof(diag_proc_info_t), cmp_proc_pid);

    ptop_proc_view_t *items =
        calloc(curr->proc_count, sizeof(ptop_proc_view_t));

    if (!items) {
        free(prev_sorted);
        return -1;
    }

    unsigned long long total_prev = diag_cpu_total(&prev->cpu);
    unsigned long long total_curr = diag_cpu_total(&curr->cpu);
    unsigned long long total_delta = total_curr - total_prev;

    if (total_delta == 0)
        total_delta = 1;

    for (size_t i = 0; i < curr->proc_count; i++) {
        const diag_proc_info_t *c = &curr->procs[i];
        const diag_proc_info_t *p =
            find_prev_proc(prev_sorted, prev->proc_count, c->pid);

        unsigned long long proc_delta = 0;

        if (p) {
            // 這裡假設 diag_proc_info_t 有 utime/stime
            proc_delta = (c->utime + c->stime) - (p->utime + p->stime);
        }

        items[i].pid = c->pid;
        items[i].ppid = c->ppid;
        items[i].rss_kb = c->rss_kb;
        items[i].state = c->state;
        strncpy(items[i].comm, c->comm, sizeof(items[i].comm) - 1);

        items[i].cpu_percent = 100.0 * ((double)proc_delta / (double)total_delta);
    }

    out->items = items;
    out->count = curr->proc_count;

    free(prev_sorted);
    return 0;
}

void ptop_delta_free(ptop_delta_result_t *out)
{
    free(out->items);
    out->items = NULL;
    out->count = 0;
}
