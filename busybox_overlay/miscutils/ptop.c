//config:config PTOP
//config:	bool "ptop (diag process metadata monitor)"
//config:	default n
//config:	help
//config:	Lightweight snapshot-based process monitoring tool.
//applet:IF_PTOP(APPLET_NOEXEC(ptop, ptop, BB_DIR_USR_BIN, BB_SUID_DROP, ptop))
//kbuild:lib-$(CONFIG_PTOP) += ptop.o ptop_snapshot.o ptop_delta.o ptop_filter.o ptop_policy.o ptop_output.o ptop_render_tty.o ptop_signal.o ptop_tree.o
//kbuild:CFLAGS_ptop.o += -DBUSYBOX

//usage:#define ptop_trivial_usage
//usage:       "[OPTIONS]"
//usage:#define ptop_full_usage "\n\n"
//usage:       "Snapshot-based lightweight process monitor."
//usage:     "\n"
//usage:     "\nOptions:"
//usage:     "\n\n    -1"
//usage:     "\n        One-shot mode"
//usage:     "\n\n    -n COUNT"
//usage:     "\n        Refresh COUNT times then exit"
//usage:     "\n\n    -t N"
//usage:     "\n        Show top N processes"
//usage:     "\n\n    --output table|raw|json"
//usage:     "\n        Select output format"
//usage:     "\n"
//usage:     "\nExamples:"
//usage:     "\n    ptop --output table -1 -t 10"
//usage:     "\n    ptop --output json -1 -n 1"

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
#include <getopt.h>

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
#ifndef PTOP_ENTRY_NAME
#define PTOP_ENTRY_NAME main
#endif
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
        "  -h, --help  help\n"
        "  --output FMT table|raw|json (alias of -b|-r|-j)\n"
    );
}
#endif

/* ----------------------------
 * Helpers
 * ---------------------------- */

static int parse_sort_mode_safe(const char *s, ptop_sort_mode_t *out)
{
    if (strcmp(s, "pid") == 0) { *out = PTOP_SORT_PID; return 0; }
    if (strcmp(s, "cpu") == 0) { *out = PTOP_SORT_CPU; return 0; }
    if (strcmp(s, "rss") == 0) { *out = PTOP_SORT_RSS; return 0; }
    return -1;
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
    int rc = 0;

    cfg.delay_sec = 1;
    cfg.loops = -1;
    cfg.mode = PTOP_MODE_INTERACTIVE;
    cfg.sort_mode = PTOP_SORT_PID;
    cfg.tree = 0;
    cfg.top_n = 0;
    cfg.filter_pid = -1;
    cfg.filter_state = 0;

    int opt;
    ptop_sort_mode_t sort_mode_tmp;
    enum { OPT_OUTPUT = 256 };
    static const struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"output", required_argument, NULL, OPT_OUTPUT},
        {NULL, 0, NULL, 0}
    };

    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:n:brj1t:s:p:S:h", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'd':
            cfg.delay_sec = atoi(optarg);
            if (cfg.delay_sec <= 0) {
                PTOP_ERR("invalid delay: %s", optarg);
                return 2;
            }
            break;

        case 'n':
            cfg.loops = atoi(optarg);
            if (cfg.loops <= 0) {
                PTOP_ERR("invalid count: %s", optarg);
                return 2;
            }
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
            if (cfg.top_n < 0) {
                PTOP_ERR("invalid top value: %s", optarg);
                return 2;
            }
            break;

        case 's':
            if (parse_sort_mode_safe(optarg, &sort_mode_tmp) != 0) {
                PTOP_ERR("invalid sort mode: %s", optarg);
                return 2;
            }
            cfg.sort_mode = sort_mode_tmp;
            break;
        case 'p':
            cfg.filter_pid = (pid_t)atoi(optarg);
            if (cfg.filter_pid <= 0) {
                PTOP_ERR("invalid pid: %s", optarg);
                return 2;
            }
            break;
        case 'S':
            if (optarg[0] == '\0' || optarg[1] != '\0') {
                PTOP_ERR("invalid state: %s", optarg);
                return 2;
            }
            cfg.filter_state = (char)toupper((unsigned char)optarg[0]);
            break;

        case OPT_OUTPUT:
            if (strcmp(optarg, "table") == 0) {
                cfg.mode = PTOP_MODE_BATCH;
            } else if (strcmp(optarg, "raw") == 0) {
                cfg.mode = PTOP_MODE_RAW;
            } else if (strcmp(optarg, "json") == 0) {
                cfg.mode = PTOP_MODE_JSON;
            } else {
                PTOP_ERR("invalid output mode: %s", optarg);
                return 2;
            }
            break;

        case 'h':
            PTOP_USAGE();
            return 0;
        case '?':
        default:
            PTOP_USAGE();
            return 2;
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
            rc = 1;
            break;
        }

        if (ptop_delta_compute(&prev, &curr, &delta) < 0) {
            PTOP_ERR("failed to compute delta");
            ptop_snapshot_free(&curr);
            rc = 1;
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

    return rc;
}
