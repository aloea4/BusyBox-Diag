# 🛠️ BusyBox-Diag

> **專為嵌入式環境打造的輕量整合式 Linux 系統診斷工具組**

[![GitHub Release](https://img.shields.io/github/v/release/aloea4/BusyBox-Diag?style=flat-square)](https://github.com/aloea4/BusyBox-Diag)

---

## 📌 專題成果

[cite_start]在資源受限的 Linux 嵌入式環境中，傳統的系統診斷工具（如 top、df、ss 等）彼此獨立，不僅需要安裝多套工具，且各自依賴不同的函式庫，導致部署成本過高且介面不一致 [cite: 20, 21]。

[cite_start]**BusyBox-Diag** 是一套以 BusyBox applet 機制為核心設計的輕量級解決方案 [cite: 23][cite_start]。本專題成功在單一的 BusyBox binary 中，整合了 Process（行程）、Filesystem（檔案系統）與 Network（網路）三大領域的完整診斷能力 [cite: 2][cite_start]。系統底層共用統一的量測層（`libdiag`），並提供一致的輸出介面，旨在為 IoT 裝置、路由器韌體與容器環境提供低開銷、高整合度的系統監控基礎設施 [cite: 4, 22]。

---

## 🌟 核心設計亮點

* [cite_start]**🚫 零外部依賴原則：** 系統直接讀取 Kernel 提供的介面（如 `/proc` 與 `statfs()`），完全不依賴 `libprocps`、`libnl` 或 `ncurses` 等外部函式庫，確保在最小化 rootfs 環境下依然能穩定運作 [cite: 24]。
* [cite_start]**📦 統一部署模型：** 採用 BusyBox applet 整合模式，最終產出單一執行檔 [cite: 25][cite_start]。透過 `argv[0]` dispatch 機制，以極低的磁碟與記憶體開銷提供完整的指令列服務 [cite: 22]。
* [cite_start]**🤖 腳本與自動化友善：** 所有工具均支援 `table`（人機互動）、`raw`（Shell Pipeline 串接）與 `json`（自動化監控與 dashboard 整合）三種輸出格式 [cite: 26, 112]。
* [cite_start]**🏗️ 共用量測層架構：** 獨立出 `libdiag` 作為底層資料採集基礎設施，統一封裝 procfs 與 statfs 的解析邏輯，大幅降低重複實作風險與維護成本 [cite: 27, 93]。

---

## 🧰 三大診斷工具集

### 1. `ptop`：輕量行程資源分析器
專為自動化監控與腳本化採樣設計的行程監控器。
* [cite_start]**Snapshot-Delta 架構：** 捨棄傳統 top 沉重的 ncurses 互動介面，透過雙時間點快照差分計算，精準還原 CPU 的真實使用率趨勢 [cite: 3, 108, 110]。
* [cite_start]**穩健的配對機制：** 採用對前一份快照排序並以二元搜尋配對 PID 的策略，穩定處理 Linux 行程動態生成的生命週期 [cite: 200]。

### 2. `diagfs`：檔案系統健康檢測工具
無需依賴外部工具即可進行空間與 metadata 監控。
* [cite_start]**深度元資料解析：** 直接呼叫 `statfs()` 提供空間使用、inode 狀態與 extent 分佈資訊 [cite: 3][cite_start]。針對 inode 使用率採用 Ceiling（無條件進位）策略，避免因整數截斷而低估風險 [cite: 213]。
* [cite_start]**Namespace 感知能力：** 透過解析 `/proc/self/mounts`，確保在 Docker 等隔離環境中，能精確反映當前容器視角下的掛載點狀態 [cite: 253]。

### 3. `diagnet`：網路連線狀態監控器
[cite_start]被動式核心 socket 狀態觀察器，不介入核心排程、不主動發送封包 [cite: 257]。
* [cite_start]**串流管線與 $O(1)$ 空間複雜度：** 在 `--stats` 純統計模式下，採用單次串流聚合設計，記憶體佔用退化為常數級別，完美適應嵌入式場景 [cite: 267, 304, 310]。
* [cite_start]**啟發式安全觀察：** 內建策略層，能根據自訂規則為主機自動標記可疑的監聽埠（Suspicious listening port）[cite: 257]。

---

## 🚀 效能與驗證成果

BusyBox-Diag 在維持輕量與低相依性的同時，效能表現依然優異。經 200 次迭代的基準測試驗證（對照基準為 GNU `top`、`df`、`ss`）：
* [cite_start]**執行速度：** ptop 中位數比值 0.868、diagfs 0.986、diagnet 1.196 [cite: 334]。
* [cite_start]**指標達成：** 三項工具均完全符合「效能差距低於 50% (比值 $\le 1.50$)」的嚴格專案目標 [cite: 328, 330]。
* [cite_start]**資源控制：** 測試期間最大常駐記憶體（Max RSS）極低（如 diagfs 僅需 2432 KB），展現其在資源受限環境下的巨大優勢 [cite: 391]。

---

## 👥 開發團隊

本開源專題由國立彰化師範大學團隊協作開發：
* **鄺采霓**：專題規劃 / `diagfs` 工具開發
* **施丞澤**：`libdiag` 共用模組開發 / BusyBox 核心整合
* **凌浩雲**：`ptop` 工具開發
* **方梓寧**：`diagnet` 工具開發

*(詳細原始碼與操作手冊，請點擊上方 GitHub 徽章前往專案主頁查閱)*