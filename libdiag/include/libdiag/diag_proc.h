#ifndef LIBDIAG_DIAG_PROC_H
#define LIBDIAG_DIAG_PROC_H

#include "libdiag/diag_common.h"

typedef struct {
    int pid;
    int ppid;
    char state;
    unsigned long rss_kb;
    unsigned long utime;
    unsigned long stime;
    char comm[DIAG_NAME_MAX];
} diag_proc_info_t;

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} diag_cpu_snapshot_t;

int diag_proc_parse_stat_line(const char *line, diag_proc_info_t *out);

int diag_proc_read(int pid, diag_proc_info_t *out);
int diag_proc_foreach(
    int (*callback)(const diag_proc_info_t *info, void *user),
    void *user
);

int diag_cpu_read_snapshot(diag_cpu_snapshot_t *out);
unsigned long long diag_cpu_total(const diag_cpu_snapshot_t *s);
unsigned long long diag_cpu_idle(const diag_cpu_snapshot_t *s);

#endif