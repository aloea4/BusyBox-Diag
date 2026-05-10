System Diagnostics Toolkit: ptop (輕量級行程分析器)
📌 專案簡介
ptop (Process Top) 是本系統診斷工具集（System Diagnostics Toolkit）中的核心應用程式之一。它專為嵌入式 Linux 環境（如 BusyBox、OpenWrt）設計，提供類似 htop 的即時系統監控功能。

本工具完全不依賴肥大的 ncurses 函式庫，而是透過純 C 語言與 ANSI 終端控制碼實現無閃爍的畫面刷新，並以模組化的方式與團隊共用的靜態函式庫 libdiag.a 進行深度整合。

✨ 核心功能與技術亮點
輕量化即時監控 (Real-time Monitoring)：

每秒自動刷新全域 CPU 使用率與實體記憶體 (RAM) 狀態。

透過精確計算兩次 CPU 快照 (Snapshot) 的 Time Ticks 差值，得出準確的真實 CPU 負載。

行程樹狀視覺化 (PID Tree Visualization)：

自動解析行程間的父子關係 (Parent-Child Relationship)。

利用遞迴演算法 (Recursive Algorithm) 在終端機繪製直觀的階層樹狀圖。

高效能動態記憶體管理 (Dynamic Memory Scaling)：

針對數量未知的 Process，實作了「容量倍增 (Double Capacity)」的動態陣列擴充演算法 (realloc)。

確保在有限的嵌入式記憶體環境下，以最低的效能開銷完成資料收集，且無記憶體洩漏 (Memory Leak)。

無縫系統整合 (Seamless Integration)：

完全基於團隊制定的 diag_proc.h 介面開發，利用 Callback Function (diag_proc_foreach) 達成低耦合的資料交換。

📂 檔案架構
本工具的程式碼位於 tools/ 目錄下，並依賴 src/libdiag/ 的底層實作：

Plaintext
.
├── include/libdiag/
│   ├── diag_proc.h       # 行程資料結構與 API 介面 (依賴)
│   └── diag_common.h     # 共用定義
├── src/libdiag/          # 底層資料收集實作 (編譯為 libdiag.a)
├── tools/
│   └── ptop.c            # 本工具主程式原始碼
└── Makefile              # 專案編譯腳本
🚀 編譯與執行指南
編譯環境要求
GCC 編譯器

支援 POSIX 標準的 Linux 作業系統 (或 WSL)

編譯步驟
請在專案根目錄下執行 make 指令，Makefile 會自動先編譯底層的 libdiag.a，接著再編譯 ptop 工具：

Bash
# 清理舊的編譯檔案 (可選)
make clean

# 編譯所有專案 (包含 libdiag 與 tools/ptop)
make all
執行方式
編譯完成後，執行檔會生成在 tools/ 目錄下：

Bash
./tools/ptop
提示：若要檢視所有系統層級的 Process，建議以 root 權限執行 (sudo ./tools/ptop)。

操作說明
程式啟動後會自動隱藏游標，並每隔 1 秒重新整理畫面。

若要退出程式，請在鍵盤按下 Ctrl + C，程式會捕捉中斷訊號 (SIGINT)，安全釋放記憶體並優雅退出。

🧠 演算法設計說明 (給開發者與維護者)
CPU 快照計算：
為了計算 CPU 使用率，程式會在主迴圈中保留前一秒的狀態 (cpu_prev)，並與當下狀態 (cpu_curr) 相減。公式為：
CPU Usage = (Total Delta - Idle Delta) / Total Delta * 100%

尋找樹狀圖根節點 (Root Discovery)：
程式實作了 is_root_process 函式。對於每一個行程，如果在當前收集到的列表中「找不到它的 PPID (父行程 ID)」，或者其 PPID 為系統初始行程 (如 PID 0, 1, 2)，則將其標記為樹狀圖的頂點，並從該點啟動深度優先搜尋 (DFS) 進行畫面渲染。
