# ptop 深度診斷報告 v2

## Snapshot-based Linux Process Monitoring Tool

## 0. Tool 正確定位

`ptop` 不應被定位成：

```text
htop clone
terminal animation tool
彩色 process tree demo
```

真正應該定位成：

```text
ptop 是 BusyBox-Diag 系統診斷工具集中的
snapshot-based Linux process monitoring tool。

它透過 Linux procfs 與 libdiag process interface
收集 process metadata、CPU snapshot 與 memory summary，
並進行 snapshot delta analysis、process hierarchy reconstruction、
resource usage analysis 與 terminal / batch presentation。
```

也就是：

```text
Linux process state
        ↓
procfs export
        ↓
libdiag measurement layer
        ↓
snapshot reconstruction
        ↓
delta analysis
        ↓
process hierarchy analysis
        ↓
filter / sort / policy
        ↓
TTY / raw / JSON output
```

這樣才真正符合：

```text
UNIX process monitoring architecture
```

而不是：

```text
畫 process tree 的 terminal tool
```

---

# 1. 核心概念模型：ptop 的本質其實是 snapshot system

這是 `ptop` 最重要的概念。

很多人以為：

```text
top / htop 是一直讀 /proc 然後 repaint terminal
```

其實不對。

真正 process monitor 的核心是：

```text
time-based snapshot analysis
```

也就是：

```text
prev snapshot
        ↓
curr snapshot
        ↓
delta correlation
        ↓
CPU / process state analysis
```

---

# 現在最大的問題

目前 `ptop` 的描述還偏向：

```text
process tree
CPU usage
memory usage
```

但：

# 沒有真正建立 snapshot abstraction。

---

# 正確模型

每次 refresh：

```text
不是 repaint terminal
```

而是：

# 建立一個：

```text
system process snapshot
```

---

# 建議 snapshot struct

```c
typedef struct {
    diag_proc_t *procs;
    size_t proc_count;

    cpu_snapshot_t cpu;
    mem_snapshot_t mem;

    struct timespec timestamp;
} ptop_snapshot_t;
```

---

# 然後：

```text
snapshot_t(prev)
        ↓
snapshot_t(curr)
        ↓
delta engine
```

---

# 為什麼這很重要？

因為：

```text
top
htop
ps -o %cpu
```

全部都依賴：

```text
time delta
```

不是：

```text
單次 procfs parsing
```

---

# 修正方向

新增：

```text
ptop_snapshot.c
```

API：

```c
int ptop_snapshot_collect(ptop_snapshot_t *snap);
void ptop_snapshot_free(ptop_snapshot_t *snap);
```

---

# 然後：

```text
ptop_delta.c
```

專門做：

```text
process correlation + delta analysis
```

---

# 不要：

```text
main loop 到處散 snapshot code
```

---

# 2. POSIX-style API 與 Linux-specific backend 邊界

這是目前 `ptop` 最缺少的一段。

`ptop` 不應宣稱：

```text
portable process monitor
```

因為它高度依賴：

```text
/proc/[pid]/stat
/proc/stat
/proc/meminfo
```

這些都是：

```text
Linux procfs-specific
```

---

# 正確定位

```text
ptop 採用 POSIX-style CLI 與 terminal tool 設計，
但目前 backend 依賴 Linux procfs metadata。
```

---

# 建議新增章節

```markdown
## Portability Boundary

ptop follows a POSIX-style command-line and terminal tool design,
but its current backend is Linux-specific.

Portable / POSIX-style behavior:
- stdout / stderr separation
- exit code contract
- signal-safe terminal restore
- batch / raw / JSON output model

Linux-specific backend:
- /proc/[pid]/stat
- /proc/stat
- /proc/meminfo
- Linux process state model
```

---

# 為什麼？

因為：

```text
POSIX 並沒有規定 /proc
```

你要明確知道：

```text
CLI contract 可以 POSIX-style
backend 可以 Linux-specific
```

---

# 3. libdiag Boundary：libdiag 是 measurement provider，不是 monitor

這點非常重要。

目前你有：

```text
diag_proc.h
process callback
```

方向是對的。

但：

# boundary 還不夠清楚。

---

# 正確分工

## libdiag

```text
- parse /proc/[pid]
- 提供 raw process metadata
- 提供 CPU raw snapshot
- 提供 memory raw snapshot
- 回傳 struct
```

---

## ptop

```text
- snapshot reconstruction
- delta correlation
- process hierarchy
- sorting
- filtering
- terminal rendering
- policy
```

---

# 不應該讓 libdiag 做的事

```text
- CPU%
- process tree
- top-N sorting
- color
- terminal rendering
- filter policy
```

---

# 原因

因為：

```text
libdiag = measurement layer
ptop = monitoring subsystem
```

這是 library boundary design。

---

# 4. Measurement / Analysis / Policy / Presentation 分層

目前 `ptop` 最大問題：

# terminal rendering 汙染 architecture

你很可能：

```text
一邊分析 process
一邊 printf ANSI
```

這會讓：

```text
analysis layer
```

和：

```text
TTY backend
```

混在一起。

---

# 建議統一四層架構

```text
Layer 1: Measurement
Layer 2: Analysis
Layer 3: Policy
Layer 4: Presentation
```

---

# 建議檔案結構

```text
tools/ptop/
├── ptop.c                 # main loop / getopt
├── ptop_snapshot.c        # snapshot collection
├── ptop_delta.c           # CPU/process delta analysis
├── ptop_tree.c            # process hierarchy reconstruction
├── ptop_filter.c          # filtering / sorting
├── ptop_policy.c          # high cpu/rss policy
├── ptop_render_tty.c      # ANSI / terminal rendering
├── ptop_output.c          # raw / json / batch
├── ptop_signal.c          # signal-safe restore
├── ptop.h
└── README.md
```

---

# 每層責任

| 層級           | 責任                   | 不應該做        |
| ------------ | -------------------- | ----------- |
| Measurement  | 讀 /proc              | 不算 CPU%     |
| Analysis     | snapshot correlation | 不 printf    |
| Policy       | top-N / warning      | 不畫 terminal |
| Presentation | ANSI / JSON / raw    | 不重新分析資料     |

---

# 為什麼？

因為：

```text
TTY rendering 只是 frontend
```

不是：

```text
monitor 核心
```

---

# 5. CPU 模型：目前只做到 system-wide CPU

這是目前概念最容易出錯的地方。

現在 README 有：

```text
CPU Usage = (Total Delta - Idle Delta) / Total Delta
```

這是：

```text
system-wide CPU usage
```

---

# 但：

很多人會誤會：

```text
process CPU%
```

也是這樣。

實際上：

# 完全不同。

---

# 真正 process CPU%

需要：

```text
per-process utime/stime delta
```

也就是：

```text
prev snapshot
        ↓
curr snapshot
        ↓
PID correlation
        ↓
tick delta
```

---

# 這代表：

process monitor 真正核心其實是：

```text
snapshot correlation engine
```

---

# 建議 struct

```c
typedef struct {
    pid_t pid;

    unsigned long prev_utime;
    unsigned long prev_stime;

    unsigned long curr_utime;
    unsigned long curr_stime;

    double cpu_percent;
} ptop_proc_delta_t;
```

---

# API

```c
void ptop_process_delta_compute(
    const ptop_snapshot_t *prev,
    const ptop_snapshot_t *curr
);
```

---

# 為什麼重要？

因為：

真正 monitoring tool：

核心不是：

```text
讀 /proc
```

而是：

```text
分析時間差
```

---

# 6. Process hierarchy：目前很可能是 O(n²)

現在 README：

```text
遞迴尋找 parent-child
```

方向對。

但：

# 大概率是 O(n²)

也就是：

```text
每個 process
    ↓
掃全部 process 找 child
```

---

# process 多時會很糟。

真正 process monitor：

應建立：

```text
PID index
```

---

# 建議 struct

```c
typedef struct {
    pid_t pid;
    size_t index;
} pid_index_t;
```

---

# collect 完：

建立：

```text
pid → process*
```

mapping。

---

# 更好的設計

```c
typedef struct ptop_node {
    diag_proc_t raw;

    struct ptop_node *parent;

    struct ptop_node **children;
    size_t child_count;

    int depth;
} ptop_node_t;
```

---

# 為什麼？

因為：

process tree：

本質是：

```text
hierarchical graph reconstruction
```

不是：

```text
recursive print demo
```

---

# 7. Process abstraction：不要直接 everywhere 用 diag_proc_t

這是 architecture 問題。

`diag_proc_t`：

其實是：

```text
raw kernel-export metadata
```

但：

`ptop` 真正需要的是：

```text
analyzed process state
```

---

# 建議新增

```c
typedef struct {
    diag_proc_t raw;

    double cpu_percent;

    int depth;

    int is_root;

    unsigned int flags;
} ptop_process_t;
```

---

# 原因

monitor layer：

不應直接等於：

```text
kernel metadata layer
```

---

# 8. POSIX Terminal Model：ANSI escape sequence 不夠

目前 README：

```text
ANSI escape sequence
```

這太表面。

真正 terminal tool：

至少應考慮：

```text
isatty()
termios
SIGWINCH
signal-safe restore
TTY consistency
```

---

# 建議新增

## SIGWINCH support

```c
ioctl(STDOUT_FILENO, TIOCGWINSZ, ...)
```

---

# 原因

真正：

```text
top / htop
```

會跟 terminal size 綁定。

---

# 另外：

terminal backend：

不應直接 assume：

```text
stdout 永遠是 TTY
```

---

# 應：

```c
isatty(STDOUT_FILENO)
```

決定是否進 interactive mode。

---

# 9. Signal-safe terminal restore

目前：

```text
Ctrl+C safely exit
```

太表面。

真正重要的是：

```text
不要把 terminal 弄壞
```

---

# 建議

新增：

```text
SIGINT
SIGTERM
SIGSEGV
```

restore handler。

---

# 使用：

```c
atexit()
```

保底 restore。

---

# 原因

terminal tool 最怕：

```text
cursor 不見
raw mode 沒恢復
terminal state corrupted
```

---

# 10. Filtering / Sorting：monitoring tool 本質是 query engine

目前：

```text
全列 process
```

太弱。

真正 monitoring tool：

核心之一：

```text
state ranking + query
```

---

# 建議 filter struct

```c
typedef struct {
    pid_t pid;

    uid_t uid;

    char state;

    int tree_only;

    int user_only;

    int sort_mode;
} ptop_filter_t;
```

---

# sort mode

```text
cpu
rss
pid
state
```

---

# pipeline

```text
collect
    ↓
analyze
    ↓
filter
    ↓
sort
    ↓
render
```

---

# 為什麼？

因為：

```text
monitoring tool = query engine
```

不是：

```text
terminal animation
```

---

# 11. Memory model：目前過於簡化

現在只有：

```text
RAM usage
```

但 Linux memory：

不是：

```text
free vs used
```

這麼簡單。

---

# 至少應拆：

```text
MemTotal
MemFree
Buffers
Cached
SwapTotal
SwapFree
```

---

# README 應明確寫：

```text
目前 memory summary 為 simplified Linux memory model。
```

---

# 否則：

會過度宣稱。

---

# 12. UNIX / BusyBox CLI Contract

`ptop` 不應該只有 interactive UI。

真正 UNIX tool：

```text
machine-readable first
```

---

# 建議新增

```bash
ptop --once
ptop --batch
ptop --raw
ptop --json
```

---

# interactive UI：

只是：

```text
one frontend
```

---

# BusyBox 工具真正重視：

```text
small
stable
predictable
pipeable
```

不是：

```text
fancy UI
```

---

# 13. Benchmark 對照

期末專題應該與現有工具交叉驗證。

| ptop 功能             | 對照工具     |
| ------------------- | -------- |
| process list        | `ps`     |
| process tree        | `pstree` |
| CPU usage           | `top`    |
| memory summary      | `free`   |
| batch monitor       | `top -b` |
| interactive monitor | `htop`   |

---

# 建議報告加入

```text
ptop 的 process list 會與 ps/pstree 結果交叉驗證；
CPU usage 會與 top 的 system-wide CPU snapshot 比較；
memory summary 則與 free 的輸出進行比對。
```

---

# 14. 修改優先順序

## P0：一定要修

```text
1. 建立真正 snapshot abstraction
2. 建立 process delta engine
3. 明確區分 POSIX-style CLI 與 Linux procfs backend
4. Measurement / Analysis / Policy / Presentation 分層
5. libdiag 只做 measurement
6. terminal rendering 完全獨立
7. signal-safe terminal restore
```

## P1：強烈建議

```text
1. PID index 避免 O(n²)
2. process filtering / sorting engine
3. batch/raw/json mode
4. SIGWINCH + terminal resize
5. memory model 擴充
6. benchmark 對照 ps/top/htop/free
```

## P2：future work

```text
1. ncurses backend
2. cgroup awareness
3. namespace awareness
4. per-thread monitoring
5. remote process monitor
```

---

# 15. 最終 README 定位

可直接改成：

```text
ptop 是 BusyBox-Diag 系統診斷工具集中的
lightweight snapshot-based Linux process monitoring tool。

它透過 Linux procfs 與 libdiag process interface
收集 process metadata、CPU snapshot 與 memory summary，
並進行：

- snapshot delta analysis
- process hierarchy reconstruction
- process filtering / sorting
- resource usage analysis
- terminal / batch presentation

等操作。

libdiag 負責 raw measurement；
ptop 負責 monitoring analysis、policy 與 presentation。

本工具採用被動式 process state analysis，
透過 Linux procfs 匯出的 process metadata
觀察目前系統中的 process hierarchy 與 resource usage state。
```

---

# 16. Reference

1. `ptop_update.md` 已指出 `ptop` 的核心應從 terminal UI 轉向 snapshot-based process monitoring architecture。
2. `ptop_update.md` 已提出 snapshot system、delta analysis、PID index、process hierarchy reconstruction 與 terminal rendering 分層設計。
3. 課程內容包含 Process / Program Execution / Signal / Process Control，因此 `ptop` 應聚焦於 snapshot analysis、process hierarchy、signal-safe terminal handling 與 process abstraction。
4. Lesson 2 UNIX 哲學強調 Do one thing well、Pipe、Everything is a file，因此 `ptop` 不應只是一個 interactive terminal UI，而應具備 batch / raw / JSON 等 machine-readable 行為。
5. POSIX terminal programming 應考慮 `isatty()`、`termios`、`SIGWINCH`、signal-safe restore 與 terminal state consistency，而不只是 ANSI escape sequence。

