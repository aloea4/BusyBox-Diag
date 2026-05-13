#include "ptop.h"
#include <stdio.h>

static void output_raw(const ptop_snapshot_t *snap, const ptop_delta_result_t *delta)
{
    printf("ptop_raw ts=%lld.%09ld procs=%zu cpu=%.1f mem_total_kb=%lu mem_free_kb=%lu buffers_kb=%lu cached_kb=%lu swap_total_kb=%lu swap_free_kb=%lu\n",
           (long long)snap->ts.tv_sec,
           snap->ts.tv_nsec,
           snap->proc_count,
           delta->system_cpu_percent,
           snap->mem_total_kb,
           snap->mem_free_kb,
           snap->buffers_kb,
           snap->cached_kb,
           snap->swap_total_kb,
           snap->swap_free_kb);

    for (size_t i = 0; i < delta->count; i++) {
        printf("pid=%d ppid=%d comm=%s state=%c rss_kb=%lu cpu=%.2f\n",
               delta->items[i].pid,
               delta->items[i].ppid,
               delta->items[i].comm,
               delta->items[i].state,
               delta->items[i].rss_kb,
               delta->items[i].cpu_percent);
    }
}

static void output_json(const ptop_snapshot_t *snap, const ptop_delta_result_t *delta)
{
    printf("{\"timestamp\":{\"sec\":%lld,\"nsec\":%ld},\"process_count\":%zu,"
           "\"system_cpu_percent\":%.2f,"
           "\"memory\":{\"mem_total_kb\":%lu,\"mem_free_kb\":%lu,\"buffers_kb\":%lu,\"cached_kb\":%lu,\"swap_total_kb\":%lu,\"swap_free_kb\":%lu},"
           "\"processes\":[",
           (long long)snap->ts.tv_sec,
           snap->ts.tv_nsec,
           snap->proc_count,
           delta->system_cpu_percent,
           snap->mem_total_kb,
           snap->mem_free_kb,
           snap->buffers_kb,
           snap->cached_kb,
           snap->swap_total_kb,
           snap->swap_free_kb);

    for (size_t i = 0; i < delta->count; i++) {
        if (i != 0)
            printf(",");
        printf("{\"pid\":%d,\"ppid\":%d,\"comm\":\"%s\",\"state\":\"%c\",\"rss_kb\":%lu,\"cpu_percent\":%.2f}",
               delta->items[i].pid,
               delta->items[i].ppid,
               delta->items[i].comm,
               delta->items[i].state,
               delta->items[i].rss_kb,
               delta->items[i].cpu_percent);
    }
    printf("]}\n");
}

int ptop_output_batch(const ptop_snapshot_t *snap,
                      const ptop_delta_result_t *delta,
                      const ptop_config_t *cfg)
{
    if (!snap || !delta || !cfg)
        return -1;

    if (cfg->mode == PTOP_MODE_RAW) {
        output_raw(snap, delta);
        return 0;
    }

    if (cfg->mode == PTOP_MODE_JSON) {
        output_json(snap, delta);
        return 0;
    }

    printf("ptop_batch ts=%lld.%09ld procs=%zu cpu=%.1f mem_total_kb=%lu mem_free_kb=%lu buffers_kb=%lu cached_kb=%lu swap_total_kb=%lu swap_free_kb=%lu\n",
           (long long)snap->ts.tv_sec,
           snap->ts.tv_nsec,
           snap->proc_count,
           delta->system_cpu_percent,
           snap->mem_total_kb,
           snap->mem_free_kb,
           snap->buffers_kb,
           snap->cached_kb,
           snap->swap_total_kb,
           snap->swap_free_kb);

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
