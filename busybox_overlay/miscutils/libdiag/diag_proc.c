#include "libdiag/diag_proc.h"
#include "libdiag/diag_common.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_pid_name(const char *name)
{
    size_t i;

    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (i = 0; name[i] != '\0'; i++) {
        if (!isdigit((unsigned char)name[i])) {
            return 0;
        }
    }

    return 1;
}

static int read_file_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp;

    if (path == NULL || buf == NULL || size == 0) {
        return DIAG_ERR_INVALID_ARG;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return DIAG_ERR_IO;
    }

    if (fgets(buf, (int)size, fp) == NULL) {
        fclose(fp);
        return DIAG_ERR_IO;
    }

    fclose(fp);
    return DIAG_OK;
}

int diag_proc_parse_stat_line(const char *line, diag_proc_info_t *out)
{
    const char *lparen;
    const char *rparen;
    const char *after;
    char comm_buf[DIAG_NAME_MAX];
    size_t comm_len;
    int matched;

    if (line == NULL || out == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    lparen = strchr(line, '(');
    rparen = strrchr(line, ')');

    if (lparen == NULL || rparen == NULL || rparen <= lparen) {
        return DIAG_ERR_PARSE;
    }

    comm_len = (size_t)(rparen - lparen - 1);
    if (comm_len >= sizeof(comm_buf)) {
        comm_len = sizeof(comm_buf) - 1;
    }

    memcpy(comm_buf, lparen + 1, comm_len);
    comm_buf[comm_len] = '\0';

    /*
     * /proc/[pid]/stat:
     * pid (comm) state ppid pgrp session tty_nr tpgid flags
     * minflt cminflt majflt cmajflt utime stime ...
     *
     * after points to "state ppid ..."
     */
    after = rparen + 2;

    matched = sscanf(
        after,
        "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %lu %lu",
        &out->state,
        &out->ppid,
        &out->utime,
        &out->stime
    );

    if (matched != 4) {
        return DIAG_ERR_PARSE;
    }

    strncpy(out->comm, comm_buf, sizeof(out->comm) - 1);
    out->comm[sizeof(out->comm) - 1] = '\0';

    return DIAG_OK;
}

static int read_proc_rss_kb(int pid, unsigned long *rss_kb)
{
    char path[DIAG_PATH_MAX];
    char line[DIAG_LINE_MAX];
    FILE *fp;

    if (rss_kb == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    *rss_kb = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    fp = fopen(path, "r");
    if (fp == NULL) {
        return DIAG_ERR_IO;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            unsigned long value = 0;

            if (sscanf(line + 6, "%lu", &value) == 1) {
                *rss_kb = value;
                fclose(fp);
                return DIAG_OK;
            }

            fclose(fp);
            return DIAG_ERR_PARSE;
        }
    }

    fclose(fp);
    return DIAG_OK;
}

int diag_proc_read(int pid, diag_proc_info_t *out)
{
    char path[DIAG_PATH_MAX];
    char line[DIAG_LINE_MAX];
    int ret;

    if (pid <= 0 || out == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->pid = pid;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    ret = read_file_first_line(path, line, sizeof(line));
    if (ret != DIAG_OK) {
        return ret;
    }

    ret = diag_proc_parse_stat_line(line, out);
    if (ret != DIAG_OK) {
        return ret;
    }

    out->pid = pid;

    ret = read_proc_rss_kb(pid, &out->rss_kb);
    if (ret != DIAG_OK) {
        out->rss_kb = 0;
    }

    return DIAG_OK;
}

int diag_proc_foreach(
    int (*callback)(const diag_proc_info_t *info, void *user),
    void *user
)
{
    DIR *dir;
    struct dirent *entry;

    if (callback == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        return DIAG_ERR_IO;
    }

    while ((entry = readdir(dir)) != NULL) {
        int pid;
        int ret;
        diag_proc_info_t info;

        if (!is_pid_name(entry->d_name)) {
            continue;
        }

        pid = atoi(entry->d_name);
        if (pid <= 0) {
            continue;
        }

        ret = diag_proc_read(pid, &info);
        if (ret != DIAG_OK) {
            continue;
        }

        ret = callback(&info, user);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return DIAG_OK;
}

int diag_cpu_read_snapshot(diag_cpu_snapshot_t *out)
{
    char line[DIAG_LINE_MAX];
    FILE *fp;
    int matched;

    if (out == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        return DIAG_ERR_IO;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return DIAG_ERR_IO;
    }

    fclose(fp);

    matched = sscanf(
        line,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &out->user,
        &out->nice,
        &out->system,
        &out->idle,
        &out->iowait,
        &out->irq,
        &out->softirq,
        &out->steal
    );

    if (matched < 4) {
        return DIAG_ERR_PARSE;
    }

    return DIAG_OK;
}

unsigned long long diag_cpu_total(const diag_cpu_snapshot_t *s)
{
    if (s == NULL) {
        return 0;
    }

    return s->user
         + s->nice
         + s->system
         + s->idle
         + s->iowait
         + s->irq
         + s->softirq
         + s->steal;
}

unsigned long long diag_cpu_idle(const diag_cpu_snapshot_t *s)
{
    if (s == NULL) {
        return 0;
    }

    return s->idle + s->iowait;
}