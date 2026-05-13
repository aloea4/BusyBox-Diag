#include "ptop.h"

int ptop_policy_apply(ptop_delta_result_t *res, const ptop_config_t *cfg)
{
    if (!res || !cfg || !res->items)
        return -1;

    /* top-N cut */
    if (cfg->top_n > 0 && res->count > (size_t)cfg->top_n) {
        res->count = cfg->top_n;
    }

    return 0;
}
