#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ptop.h"

static int load_fixture(const char *path, ptop_delta_result_t *out)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    size_t cap = 8;
    out->items = calloc(cap, sizeof(*out->items));
    out->count = 0;
    out->system_cpu_percent = 42.0;
    if (!out->items) {
        fclose(fp);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;
        int pid = 0, ppid = 0;
        char state = 0;
        unsigned long rss = 0;
        double cpu = 0.0;
        char comm[64] = {0};
        if (sscanf(line, "%d %d %c %lu %lf %63s", &pid, &ppid, &state, &rss, &cpu, comm) != 6)
            continue;

        if (out->count == cap) {
            size_t new_cap = cap * 2;
            ptop_proc_view_t *p = realloc(out->items, new_cap * sizeof(*out->items));
            if (!p) {
                fclose(fp);
                return -1;
            }
            out->items = p;
            cap = new_cap;
        }

        out->items[out->count].pid = (pid_t)pid;
        out->items[out->count].ppid = (pid_t)ppid;
        out->items[out->count].state = state;
        out->items[out->count].rss_kb = rss;
        out->items[out->count].cpu_percent = cpu;
        strncpy(out->items[out->count].comm, comm, sizeof(out->items[out->count].comm) - 1);
        out->count++;
    }

    fclose(fp);
    return 0;
}

static int compare_with_expected(const ptop_delta_result_t *res, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    size_t i = 0;
    char line[64];
    while (fgets(line, sizeof(line), fp)) {
        int expected_pid = atoi(line);
        if (i >= res->count || res->items[i].pid != expected_pid) {
            fclose(fp);
            return -1;
        }
        i++;
    }
    fclose(fp);
    return (i == res->count) ? 0 : -1;
}

static void free_result(ptop_delta_result_t *res)
{
    free(res->items);
    res->items = NULL;
    res->count = 0;
}

int main(void)
{
    ptop_delta_result_t res = {0};
    ptop_config_t cfg = {0};

    if (load_fixture("tests/fixtures/proc_views.txt", &res) != 0) {
        fprintf(stderr, "load fixture failed\n");
        return 1;
    }

    cfg.filter_pid = -1;
    cfg.filter_state = 0;
    cfg.top_n = 0;
    if (ptop_filter_apply(&res, &cfg) != 0) return 1;
    if (ptop_policy_apply(&res, &cfg) != 0) return 1;
    if (compare_with_expected(&res, "tests/fixtures/expected_pid_order.txt") != 0) {
        fprintf(stderr, "pid sort expectation failed\n");
        return 1;
    }

    free_result(&res);
    memset(&res, 0, sizeof(res));
    if (load_fixture("tests/fixtures/proc_views.txt", &res) != 0) return 1;

    cfg.filter_pid = -1;
    cfg.filter_state = 0;
    cfg.top_n = 2;
    if (ptop_filter_apply(&res, &cfg) != 0) return 1;
    if (ptop_policy_apply(&res, &cfg) != 0) return 1;
    if (res.count != 2 || res.items[0].pid != 101 || res.items[1].pid != 55) {
        fprintf(stderr, "top-N expectation failed\n");
        return 1;
    }

    free_result(&res);
    memset(&res, 0, sizeof(res));
    if (load_fixture("tests/fixtures/proc_views.txt", &res) != 0) return 1;

    cfg.filter_pid = -1;
    cfg.filter_state = 'S';
    cfg.top_n = 0;
    if (ptop_filter_apply(&res, &cfg) != 0) return 1;
    if (res.count != 2 || res.items[0].pid != 55 || res.items[1].pid != 77) {
        fprintf(stderr, "state filter expectation failed\n");
        return 1;
    }

    free_result(&res);
    printf("logic tests passed\n");
    return 0;
}

