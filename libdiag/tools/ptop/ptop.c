#include "libbb.h"
#include "ptop.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char ptop_usage[] ALIGN1 =
    "ptop [OPTIONS]\n"
    "\n"
    "Snapshot-based Linux process monitoring tool.\n"
    "\n"
    "Options:\n"
    "  -d SEC     refresh delay seconds (default 1)\n"
    "  -n COUNT   refresh COUNT times then exit\n"
    "  -b         batch mode (no ANSI, one-shot output)\n"
    "  -1         once (single snapshot)\n"
    "  -t N       show top N processes (default all)\n"
    "  -s MODE    sort mode: pid|cpu|rss (default pid)\n"
    "  -h         help\n";

static ptop_sort_mode_t parse_sort_mode(const char *s)
{
    if (!strcmp(s, "pid")) return PTOP_SORT_PID;
    if (!strcmp(s, "cpu")) return PTOP_SORT_CPU;
    if (!strcmp(s, "rss")) return PTOP_SORT_RSS;

    bb_error_msg_and_die("invalid sort mode: %s", s);
}

int ptop_sort_apply(ptop_delta_result_t *delta,
                    const ptop_config_t *cfg)
{
    (void)delta;
    (void)cfg;

    /* TODO: implement sorting engine */
    return 0;
}

int ptop_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int ptop_main(int argc, char **argv)
{
    ptop_config_t cfg;
    cfg.delay_sec = 1;
    cfg.loops = -1;
    cfg.mode = PTOP_MODE_INTERACTIVE;
    cfg.sort_mode = PTOP_SORT_PID;
    cfg.tree = 0;
    cfg.top_n = 0;

    int opt;
    while ((opt = getopt(argc, argv, "d:n:bh1t:s:")) != -1) {
        switch (opt) {
        case 'd':
            cfg.delay_sec = atoi(optarg);
            if (cfg.delay_sec <= 0)
                bb_error_msg_and_die("invalid delay: %s", optarg);
            break;

        case 'n':
            cfg.loops = atoi(optarg);
            if (cfg.loops <= 0)
                bb_error_msg_and_die("invalid count: %s", optarg);
            break;

        case 'b':
            cfg.mode = PTOP_MODE_BATCH;
            break;

        case '1':
            cfg.loops = 1;
            break;

        case 't':
            cfg.top_n = atoi(optarg);
            if (cfg.top_n < 0)
                bb_error_msg_and_die("invalid top: %s", optarg);
            break;

        case 's':
            cfg.sort_mode = parse_sort_mode(optarg);
            break;

        case 'h':
        default:
            bb_show_usage();
        }
    }

    ptop_signal_init();

    int interactive = isatty(STDOUT_FILENO) && (cfg.mode == PTOP_MODE_INTERACTIVE);

    if (interactive)
        ptop_terminal_hide_cursor();

    ptop_snapshot_t prev, curr;
    ptop_delta_result_t delta;

    if (ptop_snapshot_collect(&prev) < 0) {
        bb_error_msg("failed to collect initial snapshot");
        return 1;
    }

    /* main loop */
    while (!ptop_should_exit() && (cfg.loops != 0)) {

        sleep(cfg.delay_sec);

        if (ptop_snapshot_collect(&curr) < 0) {
            bb_error_msg("failed to collect snapshot");
            break;
        }

        if (ptop_delta_compute(&prev, &curr, &delta) < 0) {
            bb_error_msg("failed to compute delta");
            ptop_snapshot_free(&curr);
            break;
        }

        ptop_filter_apply(&delta, &cfg);
        ptop_sort_apply(&delta, &cfg);
        ptop_policy_apply(&delta, &cfg);

        if (cfg.mode == PTOP_MODE_BATCH) {
            ptop_output_batch(&curr, &delta, &cfg);
            break;
        } else {
            ptop_render_tty(&curr, &delta, &cfg);
        }

        ptop_delta_free(&delta);
        ptop_snapshot_free(&prev);

        prev = curr;

        if (cfg.loops > 0)
            cfg.loops--;
    }

    ptop_snapshot_free(&prev);

    if (interactive)
        ptop_terminal_restore();

    return 0;
}
