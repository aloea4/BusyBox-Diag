#include "ptop.h"
#include <string.h>
#include <stdlib.h>

/*
 * Toybox-style behavior:
 * - filters are applied in-place
 * - stable, predictable
 */

static int match_pid(pid_t pid, const ptop_filter_t *f)
{
    if (!f->pid) return 1;
    return pid == f->pid;
}

static int match_state(char state, const ptop_filter_t *f)
{
    if (!f->state) return 1;
    return state == f->state;
}

void ptop_filter_apply(ptop_delta_result_t *res, const ptop_config_t *cfg)
{
    if (!res || !cfg) return;

    size_t w = 0;

    for (size_t i = 0; i < res->count; i++) {
        ptop_process_t *p = &res->proc[i];

        if (!match_pid(p->pid, &cfg->filter))
            continue;

        if (!match_state(p->state, &cfg->filter))
            continue;

        res->proc[w++] = *p;
    }

    res->count = w;
}
