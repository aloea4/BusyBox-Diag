#include "ptop.h"
#include <stdlib.h>
#include <string.h>

/* sort helper: diag_proc_info_t pid compare */
static int cmp_proc_pid(const void *a, const void *b)
{
    const diag_proc_info_t *pa = (const diag_proc_info_t *)a;
    const diag_proc_info_t *pb = (const diag_proc_info_t *)b;

    if (pa->pid < pb->pid) return -1;
    if (pa->pid > pb->pid) return 1;
    return 0;
}

static const diag_proc_info_t *find_prev_proc(
    const diag_proc_info_t *prev_list,
    size_t prev_count,
    pid_t pid)
{
    diag_proc_info_t key;
    memset(&key, 0, sizeof(key));
    key.pid = pid;

    return (const diag_proc_info_t *)bsearch(
        &key,
        prev_list,
        prev_count,
        sizeof(diag_proc_info_t),
        cmp_proc_pid
    );
}

int ptop_delta_compute(const ptop_snapshot_t *prev,
                       const ptop_snapshot_t *curr,
                       ptop_delta_result_t *out)
{
    memset(out, 0, sizeof(*out));

    if (!prev || !curr)
        return -1;

    /* compute system CPU usage */
    unsigned long long t_prev = diag_cpu_total(&prev->cpu);
    unsigned long long t_curr = diag_cpu_total(&curr->cpu);

    unsigned long long i_prev = diag_cpu_idle(&prev->cpu);
    unsigned long long i_curr = diag_cpu_idle(&curr->cpu);

    unsigned long long total_delta = t_curr - t_prev;
    unsigned long long idle_delta = i_curr - i_prev;

    if (total_delta == 0)
        total_delta = 1;

    out->system_cpu_percent =
        100.0 * (double)(total_delta - idle_delta) / (double)total_delta;

    /* prev snapshot copy + sort for bsearch */
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

    for (size_t i = 0; i < curr->proc_count; i++) {
        const diag_proc_info_t *c = &curr->procs[i];
        const diag_proc_info_t *p =
            find_prev_proc(prev_sorted, prev->proc_count, c->pid);

        unsigned long long proc_delta = 0;

        /* NOTE:
         * This requires diag_proc_info_t provides utime/stime.
         * If your libdiag doesn't have them, set proc_delta=0.
         */
        if (p) {
            unsigned long long prev_ticks = (unsigned long long)p->utime + (unsigned long long)p->stime;
            unsigned long long curr_ticks = (unsigned long long)c->utime + (unsigned long long)c->stime;

            if (curr_ticks >= prev_ticks)
                proc_delta = curr_ticks - prev_ticks;
        }

        items[i].pid = c->pid;
        items[i].ppid = c->ppid;
        items[i].state = c->state;
        items[i].rss_kb = c->rss_kb;

        strncpy(items[i].comm, c->comm, sizeof(items[i].comm) - 1);
        items[i].comm[sizeof(items[i].comm) - 1] = '\0';

        items[i].cpu_percent =
            100.0 * (double)proc_delta / (double)total_delta;
    }

    out->items = items;
    out->count = curr->proc_count;

    free(prev_sorted);
    return 0;
}

void ptop_delta_free(ptop_delta_result_t *out)
{
    if (!out)
        return;

    free(out->items);
    out->items = NULL;
    out->count = 0;
}
