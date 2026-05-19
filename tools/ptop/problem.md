# ptop 深度診斷報告 v2（Architecture Review）

## ⚠️ 狀態判定

這份「ptop 深度診斷報告 v2」本質上屬於：

> **架構設計規格（Architecture Specification）**
> 而不是「已完成實作系統」。

---

## 結論

- ❌ 不是全部都有做到
- ✅ 架構方向是正確的
- ⚠️ 目前實作覆蓋約 40% ～ 60%
  （主要集中在 measurement + basic rendering）

---

# 🧠 逐項對照（設計 vs 實作）

---

## 1. Snapshot-based model（❌ 未完整）

### 設計要求

- `ptop_snapshot_t` abstraction
- prev / curr snapshot model
- delta engine

### 目前實作

- ❌ 無 `ptop_snapshot_t`
- ❌ 無 `ptop_snapshot.c`
- ❌ delta logic 散落在 main loop

### 現況

👉 僅為「即時 `/proc` 讀取 + 即時計算」

### 評價

❌ 架構概念存在，但未工程化

---

## 2. POSIX / Linux boundary（⚠️ 部分完成）

### 設計要求

- POSIX-style CLI
- Linux procfs backend 明確分離

### 目前實作

- ✅ `isatty()` / signal handling
- ⚠️ 無正式 architecture boundary

### 評價

🟡 概念正確，但未制度化

---

## 3. libdiag boundary（⚠️ 半正確）

### 設計要求

- libdiag = measurement only

### 目前實作

- ✅ `/proc` parsing 在 libdiag
- ⚠️ CPU + process parsing 混合
- ❌ 無 strict boundary enforcement

### 評價

🟡 可用，但責任邊界不清晰

---

## 4. 4-layer architecture（❌ 未實作）

### 設計要求

Measurement
Analysis
Policy
Presentation

### 目前實作

- ❌ 全部集中於 `ptop.c`
- ❌ tree / filter / render / logic 混合

### 評價

❌ monolithic CLI 架構

---

## 5. CPU delta model（⚠️ 部分完成）

### 設計要求

- per-process CPU = utime/stime delta

### 目前實作

- ⚠️ system-wide CPU delta ✔
- ❌ per-process CPU% 未完成

### 評價

🟡 system monitor OK，但非完整 top 行為

---

## 6. Process tree（⚠️ 正確但低效）

### 設計要求

- PID index
- O(n) tree build

### 目前實作

- ❌ O(n²) scanning

### 評價

🟡 功能正確，但非 production-grade

---

## 7. Process abstraction（❌ 未實作）

### 設計要求

- `ptop_process_t`
- analysis layer object

### 目前實作

- ❌ 直接使用 `diag_proc_info_t`

### 評價

❌ 缺少中間抽象層

---

## 8. Terminal model（⚠️ 基本完成）

### 已實作

- ANSI escape sequences
- cursor hide/show
- signal basic restore

### 未實作

- ❌ SIGWINCH terminal resize handling

### 評價

🟡 CLI 可用，但非 robust TTY system

---

## 9. Filtering / sorting / top-N（⚠️ 半完成）

### 設計要求

- PID filter
- state filter
- sort engine
- top-N policy

### 目前實作

- ❌ filter 非模組化
- ⚠️ sorting ad-hoc
- ⚠️ top-N 無 policy layer

### 評價

🟡 功能存在，但未 engine 化

---

## 10. Memory model（⚠️ 基本）

### 目前

- VmRSS only

### 設計要求

- MemFree / Cached / Swap

### 評價

🟡 demo-level memory model

---

## 11. BusyBox / Toybox compatibility（❌ 未完成）

### 設計要求

- `ptop_main()`
- applet integration
- dual build system

### 目前實作

- ❌ applet structure incomplete
- ❌ Makefile dual mode 未完成

### 評價

❌ 尚未 BusyBox production integration

---

# 📊 最終完成度評估

| 模組 | 完成度 |
|------|--------|
| Snapshot architecture | ❌ 20% |
| libdiag measurement | 🟡 70% |
| CPU system model | 🟡 60% |
| per-process CPU | ❌ 10% |
| Process tree | 🟡 60% |
| Filtering engine | ❌ 30% |
| Policy layer | ❌ 10% |
| Rendering (TTY) | 🟡 70% |
| POSIX CLI | 🟡 60% |
| BusyBox integration | ❌ 20% |

---

# 🧠 核心結論

目前系統本質是：

> ✔ 可運行 prototype process monitor  
> ❌ 非 snapshot-based architecture system

---

# 🚨 關鍵缺口（Critical Gaps）

## 1. Missing core abstraction

- ❌ `ptop_snapshot_t`
- ❌ `ptop_process_t`
- ❌ delta engine

---

## 2. Missing architecture separation

目前：

monolithic CLI

目標：

4-layer monitoring architecture

---

## 3. Missing pipelineization

目前：

- filter / sort / tree / policy = ad-hoc code

應為：

---

## 3. Missing pipelineization

目前：

- filter / sort / tree / policy = ad-hoc code

應為：

snapshot → analysis → policy → render

---

# 🧭 一句話總結

👉 現在是：

> 「能跑的 ptop prototype」

👉 目標是：

> 「snapshot-based UNIX process monitoring system」

---

# 🚀 升級方向（Production Grade）

如果進一步完成架構升級，應補齊：

- ✔ snapshot engine
- ✔ per-process CPU delta engine
- ✔ O(n) process tree builder
- ✔ filter/sort/policy pipeline
- ✔ BusyBox applet integration
- ✔ Toybox-compatible CLI behavior
- ✔ benchmark suite（ps/top/htop comparison）
