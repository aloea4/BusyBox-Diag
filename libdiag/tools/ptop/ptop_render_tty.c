#include "ptop.h"
#include <stdio.h>

int ptop_render_tty(const ptop_snapshot_t *snap,
                    const ptop_delta_result_t *delta,
                    const ptop_config_t *cfg)
{
    (void)cfg;

    printf("\033[2J\033[H");

    printf("\033[1;32mptop (snapshot-based process monitor)\033[0m\n");
    printf("timestamp: %lld.%09ld\n",
           (long long)snap->ts.tv_sec, snap->ts.tv_nsec);

    printf("processes: %zu\n", snap->proc_count);
    printf("system cpu: %.1f%%\n", delta->system_cpu_percent);
    printf("---------------------------------------------\n");

    size_t limit = delta->count;
    if (cfg->top_n > 0 && (size_t)cfg->top_n < limit)
        limit = (size_t)cfg->top_n;

    for (size_t i = 0; i < limit; i++) {
        printf("%5d %-16s %c RSS=%7luKB CPU=%6.1f%%\n",
               delta->items[i].pid,
               delta->items[i].comm,
               delta->items[i].state,
               delta->items[i].rss_kb,
               delta->items[i].cpu_percent);
    }

    fflush(stdout);
    return 0;
}
