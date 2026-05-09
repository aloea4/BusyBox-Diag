/* tools/ptop.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "libdiag/diag_proc.h"

volatile sig_atomic_t keep_running = 1;

// 捕捉 Ctrl+C
void handle_sigint(int sig) {
    (void)sig; // 避免 unused 警告
    keep_running = 0;
}

// 用來暫存 Process 列表的動態陣列結構
typedef struct {
    diag_proc_info_t *procs;
    size_t count;
    size_t capacity;
} proc_list_t;

// 給 diag_proc_foreach 使用的 Callback 函式
int collect_proc_cb(const diag_proc_info_t *info, void *user) {
    proc_list_t *list = (proc_list_t *)user;
    
    // 如果容量不夠，進行擴充
    if (list->count >= list->capacity) {
        list->capacity = (list->capacity == 0) ? 128 : list->capacity * 2;
        diag_proc_info_t *new_procs = realloc(list->procs, list->capacity * sizeof(diag_proc_info_t));
        if (!new_procs) return -1; // 記憶體分配失敗，中斷走訪
        list->procs = new_procs;
    }
    
    list->procs[list->count++] = *info;
    return 0; // 回傳 0 讓 foreach 繼續走訪下一個 pid
}

// 判斷是否為根節點 (如果它的 ppid 不在我們當前的列表中，就視為樹的起點)
int is_root_process(proc_list_t *list, int ppid) {
    if (ppid == 0 || ppid == 1 || ppid == 2) return 1; // 1=init/systemd, 2=kthreadd
    for (size_t i = 0; i < list->count; ++i) {
        if (list->procs[i].pid == ppid) return 0;
    }
    return 1;
}

// 遞迴印出 PID 樹狀圖
void print_process_tree(proc_list_t *list, int parent_pid, int depth) {
    for (size_t i = 0; i < list->count; ++i) {
        if (list->procs[i].ppid == parent_pid) {
            // 印出縮排與分支
            for (int d = 0; d < depth; d++) printf("  | ");
            if (depth > 0) printf("|- ");
            
            // 印出各 Process 資訊 (使用組員定義的 rss_kb, comm, state)
            printf("[%5d] %-15s (State: %c, Mem: %6lu KB)\n",
                   list->procs[i].pid, list->procs[i].comm,
                   list->procs[i].state, list->procs[i].rss_kb);
            
            // 遞迴尋找子行程
            print_process_tree(list, list->procs[i].pid, depth + 1);
        }
    }
}

int main() {
    signal(SIGINT, handle_sigint);

    // 準備 CPU 狀態計算 (需要兩次快照算 delta)
    diag_cpu_snapshot_t cpu_prev, cpu_curr;
    if (diag_cpu_read_snapshot(&cpu_prev) < 0) {
        fprintf(stderr, "Failed to read initial CPU snapshot.\n");
        return 1;
    }

    printf("\033[?25l"); // 隱藏游標

    while (keep_running) {
        // 1. 抓取最新的 CPU 快照並計算使用率
        diag_cpu_read_snapshot(&cpu_curr);
        unsigned long long t_curr = diag_cpu_total(&cpu_curr);
        unsigned long long t_prev = diag_cpu_total(&cpu_prev);
        unsigned long long i_curr = diag_cpu_idle(&cpu_curr);
        unsigned long long i_prev = diag_cpu_idle(&cpu_prev);
        
        double cpu_usage = 0.0;
        unsigned long long total_delta = t_curr - t_prev;
        unsigned long long idle_delta = i_curr - i_prev;
        if (total_delta > 0) {
            cpu_usage = 100.0 * (double)(total_delta - idle_delta) / (double)total_delta;
        }
        cpu_prev = cpu_curr; // 更新快照以供下一秒使用

        // 2. 收集所有 Process 資訊
        proc_list_t list = {NULL, 0, 0};
        diag_proc_foreach(collect_proc_cb, &list);

        // 3. 渲染畫面 (清除畫面並移到左上角)
        printf("\033[2J\033[H");
        printf("\033[1;32m--- Lightweight Process Analyzer (ptop) ---\033[0m\n");
        printf("Global CPU Usage: \033[1;33m%.1f%%\033[0m\n", cpu_usage);
        printf("Total Processes: %zu\n", list.count);
        printf("-------------------------------------------\n");

        // 4. 畫出樹狀圖
        for (size_t i = 0; i < list.count; ++i) {
            if (is_root_process(&list, list.procs[i].ppid)) {
                // 印出 Root 行程
                printf("\033[1;36m[%5d] %-15s\033[0m (State: %c, Mem: %6lu KB)\n",
                       list.procs[i].pid, list.procs[i].comm,
                       list.procs[i].state, list.procs[i].rss_kb);
                // 從此節點開始往下找子行程
                print_process_tree(&list, list.procs[i].pid, 1);
            }
        }

        // 釋放本次收集的記憶體
        free(list.procs);

        // 暫停 1 秒後刷新
        sleep(1);
    }

    // 恢復游標
    printf("\033[?25h\n");
    printf("Exiting Analyzer...\n");
    return 0;
}
