# 設計理由（Design Rationale）

## 1. 為什麼用 BusyBox applet integration？

BusyBox applet integration 可將多工具集中到單一 binary，符合嵌入式/系統工具鏈常見部署模型，降低安裝與分發成本，同時保留一致的執行入口與行為。

## 2. 為什麼 shared libdiag？

三個工具都依賴核心狀態量測，若各自重做 `/proc` 或 `statfs` 解析會產生重複與漂移。`libdiag` 讓 measurement 層集中，降低重複碼與維護風險。

## 3. 為什麼 diagnet 不主動使用 socket API？

`diagnet` 的定位不是 active network tool，而是 **Passive kernel socket state observer**。它不建立連線、不探測遠端，而是被動讀取：

- `/proc/net/tcp`
- `/proc/net/udp`

這些資料對應 kernel socket table 的當前狀態，可用低侵入方式觀察連線 metadata 與 state 分布。

## 4. 為什麼 ptop 不做 full ncurses top clone？

本專題目標是輕量可整合診斷，而非完整互動式 process UI。ptop 聚焦 snapshot、delta、腳本友好輸出，避免把複雜 UI 當成主交付風險。

## 5. 為什麼採 snapshot-delta architecture？

CPU 與 process 資源變化具時間差特性，snapshot-delta 可在低複雜度下得到有意義的趨勢資訊，且容易做 batch/自動化分析。

## 6. 為什麼 raw/json/table output 很重要？

- `table`：人眼快速讀取
- `raw`：shell/awk/grep 管線
- `json`：自動化、機器解析與後續 dashboard

同時支援三者可覆蓋教學 demo、CLI 工作流與程式化整合需求。
