/* tools/ptop.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "libdiag/diag_proc.h"

volatile sig_atomic_t keep_running = 1;

// 處理 Ctrl+C 結束訊號
void handle_sigint(int sig) {
    keep_running = 0;
}

// 判斷該 PID 是否為 Root Process (沒有 Parent 在當前的列表中)
int is_root_process(diag_proc_stat_t *procs, size_t count, pid_t ppid) {
    if (ppid == 0) return 1;
    for (size_t i = 0; i < count; ++i) {
        if (procs[i].pid == ppid) {
            return 0; // 找到它的 Parent 了，所以不是 Root
        }
    }
    return 1; // 找不到 Parent，將其視為樹狀圖的頂點
}

// 遞迴印出 PID 樹狀圖
void print_process_tree(diag_proc_stat_t *procs, size_t count, pid_t parent_pid, int depth) {
    for (size_t i = 0; i < count; ++i) {
        if (procs[i].ppid == parent_pid) {
            // 處理縮排
            for (int d = 0; d < depth; d++) {
                printf("  | ");
            }
            if (depth > 0) {
                printf("|- ");
            }

            // 換算 Memory (假設 page size 為 4KB)
            long mem_kb = (procs[i].rss * 4096) / 1024;
            
            // 印出 Process 資訊
            printf("[%5d] %-15s (State: %c, Mem: %6ld KB)\n", 
                   procs[i].pid, procs[i].comm, procs[i].state, mem_kb);

            // 遞迴尋找並印出子行程
            print_process_tree(procs, count, procs[i].pid, depth + 1);
        }
    }
}

// 顯示全域系統狀態 (CPU/Mem)
void print_system_header() {
    diag_sys_stat_t sys_stat;
    if (diag_sys_get_stat(&sys_stat) == 0) {
        uint64_t total_cpu = sys_stat.cpu_user + sys_stat.cpu_nice + sys_stat.cpu_system + sys_stat.cpu_idle;
        // 簡單的 CPU 使用率計算 (實際 htop 需要記錄前一次狀態來算 delta，這邊提供輕量版概念)
        printf("\033[1;32m--- System Diagnostics: Lightweight Process Analyzer ---\033[0m\n");
        printf("Memory: %lu KB Total / %lu KB Free / %lu KB Available\n", 
               sys_stat.mem_total, sys_stat.mem_free, sys_stat.mem_available);
        printf("CPU Ticks: User %lu | Sys %lu | Idle %lu\n", 
               sys_stat.cpu_user, sys_stat.cpu_system, sys_stat.cpu_idle);
        printf("----------------------------------------------------------\n");
    }
}

int main(int argc, char **argv) {
    // 註冊中斷訊號，讓程式可以優雅退出
    signal(SIGINT, handle_sigint);

    // 隱藏游標
    printf("\033[?25l");

    while (keep_running) {
        diag_proc_stat_t *procs = NULL;
        size_t count = 0;

        if (diag_proc_get_all(&procs, &count) < 0) {
            fprintf(stderr, "Failed to read process information from libdiag.\n");
            break;
        }

        // 清空螢幕並將游標移至左上角
        printf("\033[2J\033[H");

        // 印出系統總覽
        print_system_header();

        // 尋找並印出所有的 Root Processes 及其子行程
        for (size_t i = 0; i < count; ++i) {
            if (is_root_process(procs, count, procs[i].ppid)) {
                // 發現 Root Process，將其視為樹的起點印出 (depth = 0)
                // 為了避免重複印出，我們可以暫時把 parent_pid 假定為該 ppid 來啟動遞迴
                // 但要先印出自己，再遞迴子行程
                long mem_kb = (procs[i].rss * 4096) / 1024;
                printf("\033[1;36m[%5d] %-15s\033[0m (State: %c, Mem: %6ld KB)\n", 
                       procs[i].pid, procs[i].comm, procs[i].state, mem_kb);
                
                print_process_tree(procs, count, procs[i].pid, 1);
            }
        }

        diag_proc_free_all(procs);

        // 每 2 秒刷新一次
        sleep(2);
    }

    // 恢復游標並清理螢幕
    printf("\033[?25h\n");
    printf("Analyzer exited cleanly.\n");

    return 0;
}
