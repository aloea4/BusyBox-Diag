/* tools/ptop.c */ // 這是這個工具的檔案路徑與名稱
#include <stdio.h> // 引入標準輸入輸出函式庫，用於 printf 等螢幕輸出函式
#include <stdlib.h> // 引入標準函式庫，用於動態記憶體配置 (malloc, free, realloc) 等功能
#include <unistd.h> // 引入 POSIX 標準 API 函式庫，提供 sleep() 等系統呼叫
#include <signal.h> // 引入訊號處理函式庫，用來捕捉作業系統傳來的中斷訊號 (如 Ctrl+C)
#include <string.h> // 引入字串處理函式庫，提供字串操作相關功能
#include "libdiag/diag_proc.h" // 引入組員撰寫的行程診斷模組標頭檔，取得相關資料結構與函式定義

volatile sig_atomic_t keep_running = 1; // 宣告一個全域變數作為主迴圈的開關。使用 sig_atomic_t 確保在訊號中斷時寫入的原子性，volatile 防止編譯器將其優化掉

// 捕捉 Ctrl+C
void handle_sigint(int sig) { // 定義訊號處理函式，當接收到訊號時作業系統會呼叫此函式並傳入訊號編號
    (void)sig; // 刻意將變數轉型為 void，用來避免編譯器發出「變數未使用 (unused parameter)」的警告
    keep_running = 0; // 將執行旗標設為 0，讓主迴圈在下一次條件判斷時條件不成立，進而優雅結束程式
} // 訊號處理函式結束

// 用來暫存 Process 列表的動態陣列結構
typedef struct { // 定義一個名為 proc_list_t 的結構體，用來管理不固定長度的陣列
    diag_proc_info_t *procs; // 指向 diag_proc_info_t 陣列的指標，用來存放所有收集到的行程資訊
    size_t count; // 記錄目前已經實際儲存了幾個 Process 的資訊
    size_t capacity; // 記錄目前動態配置的陣列「最大可容納數量」，以便判斷何時需要再擴充記憶體
} proc_list_t; // 結構體命名結束

// 給 diag_proc_foreach 使用的 Callback (回呼) 函式
int collect_proc_cb(const diag_proc_info_t *info, void *user) { // 每次讀取到一個 Process，底層模組就會呼叫這支函式一次
    proc_list_t *list = (proc_list_t *)user; // 將傳入的 void 泛型指標，強制轉型回我們定義的 proc_list_t 指標
    
    // 如果容量不夠，進行擴充
    if (list->count >= list->capacity) { // 檢查目前儲存的數量是否已經達到或超過陣列的物理容量
        list->capacity = (list->capacity == 0) ? 128 : list->capacity * 2; // 若容量為 0 則預設給 128；否則直接將容量翻倍 (經典的動態陣列倍增演算法)
        diag_proc_info_t *new_procs = realloc(list->procs, list->capacity * sizeof(diag_proc_info_t)); // 使用 realloc 重新向系統要一塊更大的連續記憶體空間
        if (!new_procs) return -1; // 如果系統記憶體不足導致配置失敗 (回傳 NULL)，回傳 -1 提早中斷資料收集程序
        list->procs = new_procs; // 將新配置好的記憶體起始位址，重新指派給 list 的陣列指標
    }
    
    list->procs[list->count++] = *info; // 將當前讀取到的行程內容 (*info) 複製進陣列的末端，並在賦值後將計數器 (count) 加 1
    return 0; // 回傳 0 讓 diag_proc_foreach 知道本次執行成功，可以繼續走訪下一個 Process
}

// 判斷是否為根節點 (如果它的 ppid 不在我們當前的列表中，就視為樹的起點)
int is_root_process(proc_list_t *list, int ppid) { // 定義判斷函式，傳入整個陣列列表以及要檢查的母行程 ID (ppid)
    if (ppid == 0 || ppid == 1 || ppid == 2) return 1; // 1=init/systemd, 2=kthreadd，如果是這些系統最底部的 PID，直接當作根節點 (回傳 1)
    for (size_t i = 0; i < list->count; ++i) { // 使用 for 迴圈從頭到尾走訪一次目前收集到的所有 Process
        if (list->procs[i].pid == ppid) return 0; // 如果在陣列中發現有某個行程的 PID 等於我們傳入的 ppid，代表它的老爸還活著，它就不是根節點 (回傳 0)
    }
    return 1; // 如果整個陣列都找過了卻找不到它的母行程，那就把它當作一個孤兒行程或是樹狀圖的頂點 (回傳 1)
}

// 遞迴印出 PID 樹狀圖
void print_process_tree(proc_list_t *list, int parent_pid, int depth) { // 負責印出樹狀階層的遞迴函式
    for (size_t i = 0; i < list->count; ++i) { // 走訪陣列中的每一個 Process
        if (list->procs[i].ppid == parent_pid) { // 檢查當前這個 Process 的母行程，是不是我們傳入的 parent_pid
            // 印出縮排與分支
            for (int d = 0; d < depth; d++) printf("  | "); // 依據當前遞迴的深度 (depth)，印出對應數量的垂直線作為視覺縮排
            if (depth > 0) printf("|- "); // 如果深度大於 0 (非根節點本身)，則印出橫線將節點連接起來
            
            // 印出各 Process 資訊 (使用組員定義的 rss_kb, comm, state)
            printf("[%5d] %-15s (State: %c, Mem: %6lu KB)\n", // 使用格式化輸出：PID (佔5格)、行程名稱 (左對齊佔15格)、狀態字元、記憶體 KB 數
                   list->procs[i].pid, list->procs[i].comm, // 將結構體中的 pid 與 comm 變數帶入字串
                   list->procs[i].state, list->procs[i].rss_kb); // 將結構體中的 state 與 rss_kb 變數帶入字串
            
            // 遞迴尋找子行程
            print_process_tree(list, list->procs[i].pid, depth + 1); // 將當前找到的這支行程的 PID 當成「母節點」，深度加 1，自己呼叫自己繼續往下找子節點
        }
    }
}

int main() { // 整個 C 程式的執行起點
    signal(SIGINT, handle_sigint); // 向作業系統註冊訊號處理，當使用者按下 Ctrl+C (SIGINT) 時，去執行 handle_sigint 函式

    // 準備 CPU 狀態計算 (需要兩次快照算 delta)
    diag_cpu_snapshot_t cpu_prev, cpu_curr; // 宣告兩個結構體變數，分別用來儲存「上一次」與「這一次」的 CPU 狀態快照資料
    if (diag_cpu_read_snapshot(&cpu_prev) < 0) { // 呼叫底層模組讀取最初的 CPU 快照，並判斷回傳值是否小於 0 (發生錯誤)
        fprintf(stderr, "Failed to read initial CPU snapshot.\n"); // 如果出錯，將錯誤訊息輸出到標準錯誤串流 (stderr)
        return 1; // 讓程式提早結束，並回傳 1 (非零值表示異常退出) 給作業系統
    }

    printf("\033[?25l"); // 輸出 ANSI 終端控制碼，功能為「隱藏游標」，避免畫面頻繁刷新時游標一直閃爍影響閱讀

    while (keep_running) { // 進入無窮迴圈，只要 keep_running 還是 1 就會一直執行即時監控
        // 1. 抓取最新的 CPU 快照並計算使用率
        diag_cpu_read_snapshot(&cpu_curr); // 呼叫底層模組，抓取「目前這一秒」最新的 CPU 快照存入 cpu_curr
        unsigned long long t_curr = diag_cpu_total(&cpu_curr); // 從當前快照中算出 CPU 的總運轉時間 (Total Ticks)
        unsigned long long t_prev = diag_cpu_total(&cpu_prev); // 從上一秒快照中算出 CPU 的總運轉時間 (Total Ticks)
        unsigned long long i_curr = diag_cpu_idle(&cpu_curr);  // 從當前快照中算出 CPU 的閒置時間 (Idle Ticks)
        unsigned long long i_prev = diag_cpu_idle(&cpu_prev);  // 從上一秒快照中算出 CPU 的閒置時間 (Idle Ticks)
        
        double cpu_usage = 0.0; // 宣告一個浮點數來儲存計算出來的 CPU 使用率百分比，初始值歸零
        unsigned long long total_delta = t_curr - t_prev; // 將兩次快照的總時間相減，得出這段時間內 CPU 總共前進了多少個 Ticks
        unsigned long long idle_delta = i_curr - i_prev; // 將兩次快照的閒置時間相減，得出這段時間內 CPU 有多少 Ticks 是閒置的
        if (total_delta > 0) { // 防禦性寫法，確保分母大於 0 才進行除法運算，避免程式當機
            cpu_usage = 100.0 * (double)(total_delta - idle_delta) / (double)total_delta; // 公式：(總經過時間 - 閒置經過時間) / 總經過時間 * 100%，得到真實運轉的比例
        }
        cpu_prev = cpu_curr; // 將算完的當前 CPU 快照存入 cpu_prev，作為下一次 (下一秒) 迴圈計算時的歷史依據

        // 2. 收集所有 Process 資訊
        proc_list_t list = {NULL, 0, 0}; // 宣告行程列表結構，並將指標設為 NULL、數量與容量皆設為 0 作為初始化
        diag_proc_foreach(collect_proc_cb, &list); // 將 callback 函式與陣列位址傳給底層的走訪函式，它會幫我們把所有資料塞進 list 陣列裡

        // 3. 渲染畫面 (清除畫面並移到左上角)
        printf("\033[2J\033[H"); // 輸出 ANSI 終端控制碼，\033[2J 清空整個螢幕畫面，\033[H 將游標歸位到畫面左上角 (原點)
        printf("\033[1;32m--- Lightweight Process Analyzer (ptop) ---\033[0m\n"); // 印出軟體標題，\033[1;32m 設定字體為高亮綠色，\033[0m 恢復預設顏色
        printf("Global CPU Usage: \033[1;33m%.1f%%\033[0m\n", cpu_usage); // 印出稍早算好的 CPU 使用率，保留一位小數，並用高亮黃色 (\033[1;33m) 顯示
        printf("Total Processes: %zu\n", list.count); // 印出本次迴圈總共抓到了多少支 Process，%zu 專門用來印出 size_t 型態的整數
        printf("-------------------------------------------\n"); // 印出一條分隔線讓畫面比較乾淨

        // 4. 畫出樹狀圖
        for (size_t i = 0; i < list.count; ++i) { // 跑一個 for 迴圈走訪剛剛建立好的整個 Process 陣列
            if (is_root_process(&list, list.procs[i].ppid)) { // 使用判斷函式檢查第 i 個 Process 是否為最頂層的根節點
                // 印出 Root 行程
                printf("\033[1;36m[%5d] %-15s\033[0m (State: %c, Mem: %6lu KB)\n", // 如果是根節點就印出資訊，PID 與名稱使用高亮青藍色 (\033[1;36m) 標示
                       list.procs[i].pid, list.procs[i].comm, // 帶入根節點本身的 PID 與指令名稱
                       list.procs[i].state, list.procs[i].rss_kb); // 帶入根節點本身的運行狀態與記憶體用量
                // 從此節點開始往下找子行程
                print_process_tree(&list, list.procs[i].pid, 1); // 呼叫遞迴函式，將這個根節點的 PID 視為老爸，並從深度層級 1 開始掃描子行程
            }
        } // 迴圈結束，此時畫面上已經印出完整的即時 Process 樹狀圖

        // 釋放本次收集的記憶體
        free(list.procs); // 使用 free 將前面 collect_proc_cb 中用 realloc 借來的動態記憶體空間還給系統，防止 Memory Leak (記憶體洩漏)

        // 暫停 1 秒後刷新
        sleep(1); // 讓程式休眠暫停 1 秒鐘，1 秒後會回到 while 迴圈頂端，重新抓取下一秒的系統狀態，達成動態更新效果
    }

    // 恢復游標
    printf("\033[?25h\n"); // 離開 while 迴圈後 (代表使用者按了 Ctrl+C)，輸出 ANSI 控制碼把剛剛隱藏的游標重新顯示出來
    printf("Exiting Analyzer...\n"); // 在畫面上印出提示字串，告訴使用者程式正在安全結束中
    return 0; // 主程式正常執行結束，回傳 0 給作業系統
}
