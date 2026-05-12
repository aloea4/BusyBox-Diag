/* tools/ptop.c */

#include "libbb.h"              // BusyBox 內建常用工具函式 (getopt, bb_error_msg 等)
#include <signal.h>             // signal()
#include <stdio.h>              // printf()
#include <stdlib.h>             // malloc/free/realloc
#include <unistd.h>             // sleep()

#include "libdiag/diag_proc.h"  // 你的行程資訊模組

static volatile sig_atomic_t keep_running = 1;

/* Ctrl+C 時設定旗標，讓 while loop 自然退出 */
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}

/* 動態陣列結構 */
typedef struct {
    diag_proc_info_t *procs;
    size_t count;
    size_t capacity;
} proc_list_t;

/* callback: 收集 process */
static int collect_proc_cb(const diag_proc_info_t *info, void *user)
{
    proc_list_t *list = (proc_list_t *)user;

    if (list->count >= list->capacity) {
        size_t new_capacity = (list->capacity == 0) ? 128 : list->capacity * 2;

        diag_proc_info_t *new_procs =
            realloc(list->procs, new_capacity * sizeof(diag_proc_info_t));

        if (!new_procs)
            return -1;

        list->procs = new_procs;
        list->capacity = new_capacity;
    }

    list->procs[list->count++] = *info;
    return 0;
}

/* 判斷 root process */
static int is_root_process(proc_list_t *list, int ppid)
{
    if (ppid == 0 || ppid == 1 || ppid == 2)
        return 1;

    for (size_t i = 0; i < list->count; i++) {
        if (list->procs[i].pid == ppid)
            return 0;
    }

    return 1;
}

/* 遞迴印出 process tree */
static void print_process_tree(proc_list_t *list, int parent_pid, int depth)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->procs[i].ppid == parent_pid) {

            for (int d = 0; d < depth; d++)
                printf("  | ");

            if (depth > 0)
                printf("|- ");

            printf("[%5d] %-15s (State: %c, Mem: %6lu KB)\n",
                   list->procs[i].pid,
                   list->procs[i].comm,
                   list->procs[i].state,
                   list->procs[i].rss_kb);

            print_process_tree(list, list->procs[i].pid, depth + 1);
        }
    }
}

/* BusyBox applet 入口 */
int ptop_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int ptop_main(int argc, char **argv)
{
    int delay = 1;       // 預設 1 秒刷新
    int loops = -1;      // 預設無限循環

    /* getopt: -d 秒數, -n 次數, -h */
    int opt;
    while ((opt = getopt(argc, argv, "d:n:h")) != -1) {
        switch (opt) {
        case 'd':
            delay = atoi(optarg);
            if (delay <= 0)
                bb_error_msg_and_die("invalid delay: %s", optarg);
            break;

        case 'n':
            loops = atoi(optarg);
            if (loops <= 0)
                bb_error_msg_and_die("invalid loop count: %s", optarg);
            break;

        case 'h':
        default:
            bb_show_usage();
        }
    }

    /* 註冊 Ctrl+C */
    signal(SIGINT, handle_sigint);

    /* CPU snapshot init */
    diag_cpu_snapshot_t cpu_prev, cpu_curr;
    if (diag_cpu_read_snapshot(&cpu_prev) < 0) {
        bb_error_msg("Failed to read initial CPU snapshot");
        return 1;
    }

    /* 隱藏游標 */
    printf("\033[?25l");
    fflush(stdout);

    while (keep_running && (loops != 0)) {

        /* 讀 CPU snapshot */
        if (diag_cpu_read_snapshot(&cpu_curr) < 0) {
            bb_error_msg("Failed to read CPU snapshot");
            break;
        }

        unsigned long long t_curr = diag_cpu_total(&cpu_curr);
        unsigned long long t_prev = diag_cpu_total(&cpu_prev);
        unsigned long long i_curr = diag_cpu_idle(&cpu_curr);
        unsigned long long i_prev = diag_cpu_idle(&cpu_prev);

        unsigned long long total_delta = t_curr - t_prev;
        unsigned long long idle_delta = i_curr - i_prev;

        double cpu_usage = 0.0;
        if (total_delta > 0) {
            cpu_usage = 100.0 *
                        (double)(total_delta - idle_delta) /
                        (double)total_delta;
        }

        cpu_prev = cpu_curr;

        /* 收集 process */
        proc_list_t list;
        list.procs = NULL;
        list.count = 0;
        list.capacity = 0;

        if (diag_proc_foreach(collect_proc_cb, &list) < 0) {
            bb_error_msg("Failed to collect process list");
            free(list.procs);
            break;
        }

        /* 清畫面 */
        printf("\033[2J\033[H");
        printf("\033[1;32m--- Lightweight Process Analyzer (ptop) ---\033[0m\n");
        printf("Global CPU Usage: \033[1;33m%.1f%%\033[0m\n", cpu_usage);
        printf("Total Processes: %zu\n", list.count);
        printf("-------------------------------------------\n");

        /* 印 tree */
        for (size_t i = 0; i < list.count; i++) {
            if (is_root_process(&list, list.procs[i].ppid)) {
                printf("\033[1;36m[%5d] %-15s\033[0m (State: %c, Mem: %6lu KB)\n",
                       list.procs[i].pid,
                       list.procs[i].comm,
                       list.procs[i].state,
                       list.procs[i].rss_kb);

                print_process_tree(&list, list.procs[i].pid, 1);
            }
        }

        free(list.procs);

        fflush(stdout);

        /* -n 次數控制 */
        if (loops > 0)
            loops--;

        sleep(delay);
    }

    /* 恢復游標 */
    printf("\033[?25h\n");
    printf("Exiting Analyzer...\n");

    return 0;
}
