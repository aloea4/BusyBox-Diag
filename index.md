# 🛠️ BusyBox-Diag

> [cite_start]**專為嵌入式環境打造的輕量整合式 Linux 系統診斷工具組** [cite: 8]

[![GitHub Release](https://img.shields.io/github/v/release/aloea4/BusyBox-Diag?style=flat-square)](https://github.com/aloea4/BusyBox-Diag)
[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg?style=flat-square)](https://github.com/aloea4/BusyBox-Diag)

[cite_start]傳統的 Linux 系統診斷工具 (top, df, ss) 相互獨立，難以在資源受限的嵌入式環境中一體化部署。**BusyBox-Diag** 是一套以 BusyBox applet 機制為核心的解決方案，在單一 BusyBox binary 中提供完整的 process、filesystem 與 network 系統診斷能力 [cite: 2]。

[👉 檢視完整 GitHub 專案](https://github.com/aloea4/BusyBox-Diag) | [cite_start][📖 閱讀使用手冊](https://github.com/aloea4/BusyBox-Diag/blob/main/docs/USER_MANUAL.md) [cite: 478, 480]

---

## ✨ 核心優勢 (Key Highlights)

* [cite_start]**🚫 零外部依賴：** 完全以 C 語言實作 [cite: 418][cite_start]，直接讀取 `/proc` [cite: 2] [cite_start]與 `statfs()` [cite: 13] [cite_start]系統呼叫，不依賴 `libprocps`、`libnl` 等外部函式庫 [cite: 24]。
* [cite_start]**⚙️ 單一 Binary 部署：** 完美整合進 BusyBox Applet 機制 [cite: 25]，極低的磁碟與記憶體開銷。
* [cite_start]**📊 腳本輸出友善：** 支援 `table` (人機互動)、`raw` (Shell Pipeline) 與 `json` (自動化解析) 三種輸出格式 [cite: 26]。

---

## 🧰 三大核心工具 (Features)

### 1. `ptop` - 行程資源分析器
[cite_start]捨棄沈重的 ncurses 互動介面，採用 Snapshot-Delta 架構 [cite: 106, 110][cite_start]，週期性擷取快照計算各行程的 CPU 與記憶體真實消耗，是自動化監控與腳本化採樣的最佳基礎元件 [cite: 123]。

### 2. `diagfs` - 檔案系統健康檢測
[cite_start]直接讀取 `statfs()` [cite: 13] [cite_start]提供空間使用、inode 狀態與 extent 分佈資訊。具備容器環境掛載視角感知 (Mount Namespace Awareness)，精準判定健康閾值 (OK/WARN/CRITICAL) [cite: 204, 213, 253]。

### 3. `diagnet` - 網路連線狀態監控
[cite_start]被動式核心 socket 狀態觀察器。不主動建立連線即可完整呈現 TCP/UDP 連線狀態分佈與統計摘要，支援透過啟發式規則標記可疑的監聽埠 (Suspicious listening port) [cite: 257]。

---

## 🚀 效能表現 (Performance)

在針對資源受限環境的優化下，BusyBox-Diag 的效能表現卓越。在基準測試中 (與 GNU `top`、`df`、`ss` 相比)：
* [cite_start]**執行速度：** ptop 比值 0.868、diagfs 比值 0.986、diagnet 比值 1.196 [cite: 334]。
* [cite_start]**成果判定：** 三項工具均成功達成「與對照工具維持 50% 以內效能差距」的嚴格標準，兼顧了輕量化與高效能 [cite: 330, 331]。

---

## 💻 快速安裝 (Quick Start)

採用 BusyBox overlay 整合模式，一行指令啟動自動建構：

```bash
cd ~/workspace/final_project
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1 --no-activate
