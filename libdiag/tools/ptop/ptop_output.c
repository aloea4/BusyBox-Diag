#include "ptop.h"
#include <stdio.h>

/* batch mode: machine-readable-ish output (still human friendly) */
int ptop_output_batch(const ptop_snapshot_t *snap,
                      const ptop_delta_result_t *delta,
                      const ptop_config_t *cfg)
{
    (void)cfg;

    printf("ptop_batch ts=%lld.%09ld procs=%zu cpu=%.1f\n",
           (long long)snap->ts.tv_sec,
           snap->ts.tv_nsec,
           snap->proc_count,
           delta->system_cpu_percent);

    for (size_t i = 0; i < delta->count; i++) {
        printf("pid=%d ppid=%d comm=%s state=%c rss_kb=%lu cpu=%.2f\n",
               delta->items[i].pid,
               delta->items[i].ppid,
               delta->items[i].comm,
               delta->items[i].state,
               delta->items[i].rss_kb,
               delta->items[i].cpu_percent);
    }

    return 0;
}
