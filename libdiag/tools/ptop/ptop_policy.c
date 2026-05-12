#include "ptop.h"
#include <stdlib.h>

/*
 * Toybox-like behavior:
 * - policy layer decides "what to show"
 * - not rendering
 */

static int cmp_cpu(const void *a, const void *b)
{
    const ptop_process_t *x = a;
    const ptop_process_t *y = b;

    if (x->cpu_percent < y->cpu_percent) return 1;
    if (x->cpu_percent > y->cpu_percent) return -1;
    return 0;
}

static int cmp_rss(const void *a, const void *b)
{
    const ptop_process_t *x = a;
    const ptop_process_t *y = b;

    if (x->rss_kb < y->rss_kb) return 1;
    if (x->rss_kb > y->rss_kb) return -1;
    return 0;
}

static int cmp_pid(const void *a, const void *b)
{
    const ptop_process_t *x = a;
    const ptop_process_t *y = b;

    return (x->pid - y->pid);
}

void ptop_policy_apply(ptop_delta_result_t *res, const ptop_config_t *cfg)
{
    if (!res || !cfg) return;

    /* sort */
    switch (cfg->sort_mode) {
    case PTOP_SORT_CPU:
        qsort(res->proc, res->count, sizeof(ptop_process_t), cmp_cpu);
        break;

    case PTOP_SORT_RSS:
        qsort(res->proc, res->count, sizeof(ptop_process_t), cmp_rss);
        break;

    case PTOP_SORT_PID:
    default:
        qsort(res->proc, res->count, sizeof(ptop_process_t), cmp_pid);
        break;
    }

    /* top-N cut */
    if (cfg->top_n > 0 && res->count > (size_t)cfg->top_n) {
        res->count = cfg->top_n;
    }
}
