# BusyBox-Diag CLI 相容差異決策（Draft v1）

本文件記錄「不完全相容」項目的決策：補齊、保留、或延後。

## 1. ptop `--no-header` / `--quiet`

- 現況：未提供
- 決策：**保留差異（暫不補）**
- 理由：
  - 目前 `ptop` 輸出主體已可 batch/raw/json；
  - `no-header/quiet` 對核心專題要求非阻塞。
- 後續：若 M2 要拉高一致性，可在 raw/table output 補選項。

## 2. ptop `--output` 對應 top

- 現況：`--output table|raw|json`（table alias 到 batch）
- 決策：**保留並文件化**
- 理由：
  - 專題要求三工具輸出型態趨同（table/raw/json）；
  - 與 top 完全相容不是硬性條件。

## 3. diagfs `--real/--pseudo` 與 df 型參數差異

- 現況：透過自訂旗標做 mount 類型過濾
- 決策：**保留差異（文件清楚）**
- 理由：
  - 語意可理解，且符合工具定位。

## 4. diagnet exit code 3

- 現況：help/man/README 都標示 `reserved (unused)`
- 決策：**保留 reserved，不強行實作假路徑**
- 理由：
  - 避免為了對齊而新增不自然分支。

## 5. JSON 輸出相容

- 現況：三工具均支援 JSON（格式非完全同 schema）
- 決策：**保留差異 + 穩定 schema 原則**
- 理由：
  - 專題重點是可 parse 與穩定，不要求完全共用 schema。

## 6. 下一步（M2 可執行）

1. 為矩陣每個項目補 test case ID。
2. 將 L3/L4 項目標記「保留」或「補齊」時間點。
3. README 增加「相容層聲明」小節（摘要版）。

