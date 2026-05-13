/* tools/ptop/ptop.c
 *
 * Dual-mode build:
 *
 * 1) Standalone build:
 *      gcc ... tools/ptop/ptop.c ... -o ptop
 *      -> entrypoint: main()
 *
 * 2) BusyBox applet build:
 *      compile with -DBUSYBOX
 *      -> entrypoint: ptop_main()
 */

#include "ptop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* ----------------------------
 * Dual-mode BusyBox glue
 * ---------------------------- */

#ifdef BUSYBOX
#include "libbb.h"
#define PTOP_ENTRY_NAME ptop_main
#define PTOP_DIE(...) bb_error_msg_and_die(__VA_ARGS__)
#define PTOP_ERR(...) bb_error_msg(__VA_ARGS__)
#define PTOP_USAGE() bb_show_usage()
#else
#define PTOP_ENTRY_NAME main
static void PTOP_DIE(const char *msg, const char *arg)
{
    if (arg)
        fprintf(stderr, "ptop: %s: %s\n", msg, arg);
    else
        fprintf(stderr, "ptop: %s\n", msg);
    exit(1);
}
#define PTOP_ERR(...) fprintf(stderr, "ptop: " __VA_ARGS__), fprintf(stderr, "\n")
static void PTOP_USAGE(void)
{
    printf(
        "ptop [OPTIONS]\n"
        "\n"
        "Snapshot-based Linux process monitoring tool.\n"
        "\n"
        "Options:\n"
        "  -d SEC     refresh delay seconds (default 1)\n"
        "  -n COUNT   refresh COUNT times then exit\n"
        "  -b         batch mode (no ANSI)\n"
        "  -r         raw mode (key=value lines)\n"
        "  -j         json mode\n"
        "  -1         once (single snapshot)\n"
        "  -t N       show top N processes (default all)\n"
        "  -p PID     filter by pid\n"
        "  -S STATE   filter by process state (R/S/D/Z/...)\n"
        "  -s MODE    sort mode: pid|cpu|rss (default pid)\n"
        "  -h         help\n"
    );
}
#endif

/* ----------------------------
 * Helpers
 * ---------------------------- */

static ptop_sort_mode_t parse_sort_mode(const char *s)
{
    if (strcmp(s, "pid") == 0) return PTOP_SORT_PID;
    if (strcmp(s, "cpu") == 0) return PTOP_SORT_CPU;
    if (strcmp(s, "rss") == 0) return PTOP_SORT_RSS;

#ifdef BUSYBOX
    PTOP_DIE("invalid sort mode", s);
#else
    PTOP_DIE("invalid sort mode", s);
#endif
    return PTOP_SORT_PID;
}

static int cmp_pid(const void *a, const void *b)
{
    const ptop_proc_view_t *x = (const ptop_proc_view_t *)a;
    const ptop_proc_view_t *y = (const ptop_proc_view_t *)b;
    if (x->pid < y->pid) return -1;
    if (x->pid > y->pid) return 1;
    return 0;
}

static int cmp_cpu_desc(const void *a, const void *b)
{
    const ptop_proc_view_t *x = (const ptop_proc_view_t *)a;
    const ptop_proc_view_t *y = (const ptop_proc_view_t *)b;
    if (x->cpu_percent < y->cpu_percent) return 1;
    if (x->cpu_percent > y->cpu_percent) return -1;
    return cmp_pid(a, b);
}

static int cmp_rss_desc(const void *a, const void *b)
{
    const ptop_proc_view_t *x = (const ptop_proc_view_t *)a;
    const ptop_proc_view_t *y = (const ptop_proc_view_t *)b;
    if (x->rss_kb < y->rss_kb) return 1;
    if (x->rss_kb > y->rss_kb) return -1;
    return cmp_pid(a, b);
}

int ptop_sort_apply(ptop_delta_result_t *delta, const ptop_config_t *cfg)
{
    if (!delta || !cfg || !delta->items)
        return -1;

    switch (cfg->sort_mode) {
    case PTOP_SORT_CPU:
        qsort(delta->items, delta->count, sizeof(delta->items[0]), cmp_cpu_desc);
        break;
    case PTOP_SORT_RSS:
        qsort(delta->items, delta->count, sizeof(delta->items[0]), cmp_rss_desc);
        break;
    case PTOP_SORT_PID:
    default:
        qsort(delta->items, delta->count, sizeof(delta->items[0]), cmp_pid);
        break;
    }

    return 0;
}

/* ----------------------------
 * Entry point (dual-mode)
 * ---------------------------- */

#ifdef BUSYBOX
int ptop_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
#endif

int PTOP_ENTRY_NAME(int argc, char **argv)
{
    ptop_config_t cfg;

    cfg.delay_sec = 1;
    cfg.loops = -1;
    cfg.mode = PTOP_MODE_INTERACTIVE;
    cfg.sort_mode = PTOP_SORT_PID;
    cfg.tree = 0;
    cfg.top_n = 0;
    cfg.filter_pid = -1;
    cfg.filter_state = 0;

    int opt;
    while ((opt = getopt(argc, argv, "d:n:brj1t:s:p:S:h")) != -1) {
        switch (opt) {
        case 'd':
            cfg.delay_sec = atoi(optarg);
            if (cfg.delay_sec <= 0)
                PTOP_DIE("invalid delay", optarg);
            break;

        case 'n':
            cfg.loops = atoi(optarg);
            if (cfg.loops <= 0)
                PTOP_DIE("invalid count", optarg);
            break;

        case 'b':
            cfg.mode = PTOP_MODE_BATCH;
            break;
        case 'r':
            cfg.mode = PTOP_MODE_RAW;
            break;
        case 'j':
            cfg.mode = PTOP_MODE_JSON;
            break;

        case '1':
            cfg.loops = 1;
            break;

        case 't':
            cfg.top_n = atoi(optarg);
            if (cfg.top_n < 0)
                PTOP_DIE("invalid top value", optarg);
            break;

        case 's':
            cfg.sort_mode = parse_sort_mode(optarg);
            break;
        case 'p':
            cfg.filter_pid = (pid_t)atoi(optarg);
            if (cfg.filter_pid <= 0)
                PTOP_DIE("invalid pid", optarg);
            break;
        case 'S':
            if (optarg[0] == '\0' || optarg[1] != '\0')
                PTOP_DIE("invalid state", optarg);
            cfg.filter_state = (char)toupper((unsigned char)optarg[0]);
            break;

        case 'h':
        default:
            PTOP_USAGE();
            return 0;
        }
    }

    /* signal + atexit restore */
    ptop_signal_init();

    /* If stdout is not a TTY, force batch mode (UNIX pipe-friendly). */
    int is_tty = isatty(STDOUT_FILENO);
    if (!is_tty && cfg.mode == PTOP_MODE_INTERACTIVE) {
        cfg.mode = PTOP_MODE_BATCH;
    }

    /* Hide cursor only in interactive TTY mode */
    if (cfg.mode == PTOP_MODE_INTERACTIVE) {
        ptop_terminal_hide_cursor();
    }

    ptop_snapshot_t prev;
    ptop_snapshot_t curr;
    ptop_delta_result_t delta;

    if (ptop_snapshot_collect(&prev) < 0) {
        PTOP_ERR("failed to collect initial snapshot");
        return 1;
    }

    /* main monitoring loop */
    while (!ptop_should_exit() && (cfg.loops != 0)) {

        /* delay before collecting next snapshot */
        if (cfg.delay_sec > 0)
            sleep(cfg.delay_sec);

        if (ptop_snapshot_collect(&curr) < 0) {
            PTOP_ERR("failed to collect snapshot");
            break;
        }

        if (ptop_delta_compute(&prev, &curr, &delta) < 0) {
            PTOP_ERR("failed to compute delta");
            ptop_snapshot_free(&curr);
            break;
        }

        /* pipeline: analysis -> filter -> sort -> policy -> output */
        ptop_filter_apply(&delta, &cfg);
        ptop_sort_apply(&delta, &cfg);
        ptop_policy_apply(&delta, &cfg);

        if (cfg.mode != PTOP_MODE_INTERACTIVE) {
            ptop_output_batch(&curr, &delta, &cfg);
        } else {
            ptop_render_tty(&curr, &delta, &cfg);
        }

        /* cleanup for this iteration */
        ptop_delta_free(&delta);
        ptop_snapshot_free(&prev);

        /* swap snapshot */
        prev = curr;

        if (cfg.loops > 0)
            cfg.loops--;
    }

    ptop_snapshot_free(&prev);

    if (cfg.mode == PTOP_MODE_INTERACTIVE) {
        ptop_terminal_restore();
    }

    return 0;
}
