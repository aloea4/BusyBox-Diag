#ifndef PTOP_H
#define PTOP_H

#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "libdiag/diag_proc.h"

/* ---------------------------
 * Global config / modes
 * --------------------------- */

typedef enum {
    PTOP_MODE_INTERACTIVE = 0,
    PTOP_MODE_BATCH = 1,
    PTOP_MODE_RAW = 2,
    PTOP_MODE_JSON = 3,
} ptop_mode_t;

typedef enum {
    PTOP_SORT_PID = 0,
    PTOP_SORT_CPU = 1,
    PTOP_SORT_RSS = 2,
} ptop_sort_mode_t;

typedef struct {
    int delay_sec;          /* refresh delay */
    int loops;              /* -1 = infinite */
    ptop_mode_t mode;
    ptop_sort_mode_t sort_mode;
    int tree;               /* show tree */
    int top_n;              /* 0 = all */
    pid_t filter_pid;       /* -p PID, -1 = disabled */
    char filter_state;      /* -S STATE, 0 = disabled */
} ptop_config_t;

/* ---------------------------
 * Snapshot abstraction
 * --------------------------- */

typedef struct {
    diag_proc_info_t *procs;
    size_t proc_count;

    diag_cpu_snapshot_t cpu;
    unsigned long mem_total_kb;
    unsigned long mem_free_kb;
    unsigned long buffers_kb;
    unsigned long cached_kb;
    unsigned long swap_total_kb;
    unsigned long swap_free_kb;

    struct timespec ts;
} ptop_snapshot_t;

int ptop_snapshot_collect(ptop_snapshot_t *snap);
void ptop_snapshot_free(ptop_snapshot_t *snap);

/* ---------------------------
 * Delta / analysis result
 * --------------------------- */

typedef struct {
    pid_t pid;
    pid_t ppid;

    char comm[64];
    char state;

    unsigned long rss_kb;

    double cpu_percent;
} ptop_proc_view_t;

typedef struct {
    ptop_proc_view_t *items;
    size_t count;

    double system_cpu_percent;
} ptop_delta_result_t;

int ptop_delta_compute(const ptop_snapshot_t *prev,
                       const ptop_snapshot_t *curr,
                       ptop_delta_result_t *out);

void ptop_delta_free(ptop_delta_result_t *out);

/* ---------------------------
 * Filter / sort engine
 * --------------------------- */

int ptop_filter_apply(ptop_delta_result_t *delta,
                      const ptop_config_t *cfg);

int ptop_sort_apply(ptop_delta_result_t *delta,
                    const ptop_config_t *cfg);

/* ---------------------------
 * Tree reconstruction (skeleton)
 * --------------------------- */

int ptop_tree_build(const ptop_delta_result_t *delta);

/* ---------------------------
 * Policy engine (skeleton)
 * --------------------------- */

int ptop_policy_apply(ptop_delta_result_t *delta,
                      const ptop_config_t *cfg);

/* ---------------------------
 * Presentation backends
 * --------------------------- */

int ptop_render_tty(const ptop_snapshot_t *snap,
                    const ptop_delta_result_t *delta,
                    const ptop_config_t *cfg);

int ptop_output_batch(const ptop_snapshot_t *snap,
                      const ptop_delta_result_t *delta,
                      const ptop_config_t *cfg);

/* ---------------------------
 * Signal-safe terminal restore
 * --------------------------- */

void ptop_signal_init(void);
int ptop_should_exit(void);
int ptop_terminal_resized(void);
void ptop_clear_resize_flag(void);

void ptop_terminal_hide_cursor(void);
void ptop_terminal_restore(void);

#endif
