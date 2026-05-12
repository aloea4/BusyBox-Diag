#include <stdio.h>
#include <unistd.h>
#include "ptop_delta.h"

int ptop_render_tty(const ptop_snapshot_t *snap,
                    const ptop_delta_result_t *delta)
{
    (void)delta;

    printf("\033[2J\033[H");
    printf("--- ptop snapshot monitor ---\n");
    printf("Process count: %zu\n", snap->proc_count);
    printf("Time: %lld.%09ld\n",
           (long long)snap->ts.tv_sec, snap->ts.tv_nsec);

    printf("---------------------------------\n");
    for (size_t i = 0; i < delta->count && i < 20; i++) {
        printf("%5d %-16s %c RSS=%6luKB CPU=%5.1f%%\n",
               delta->items[i].pid,
               delta->items[i].comm,
               delta->items[i].state,
               delta->items[i].rss_kb,
               delta->items[i].cpu_percent);
    }

    fflush(stdout);
    return 0;
}
