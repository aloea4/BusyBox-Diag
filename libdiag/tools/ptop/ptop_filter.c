#include "ptop.h"

int ptop_filter_apply(ptop_delta_result_t *res, const ptop_config_t *cfg)
{
    if (!res || !cfg || !res->items)
        return -1;

    size_t w = 0;
    for (size_t i = 0; i < res->count; i++) {
        const ptop_proc_view_t *p = &res->items[i];

        if (cfg->filter_pid >= 0 && p->pid != cfg->filter_pid)
            continue;
        if (cfg->filter_state != 0 && p->state != cfg->filter_state)
            continue;

        res->items[w++] = *p;
    }

    res->count = w;
    return 0;
}
