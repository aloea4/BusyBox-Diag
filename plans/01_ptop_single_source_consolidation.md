# 計畫 1：`ptop` 單一來源收斂（草案）

## 1. 背景與問題

目前 `ptop` 同時存在於：

- `tools/ptop`（主用）
- `libdiag/tools/ptop`（歷史副本）

雙份來源會導致：

- 修補/重構只改一處，另一處漂移
- 測試與文件對不齊
- Agent/新成員不易判斷哪份為真實來源

## 2. 目標

建立「唯一維護來源（SSOT）」：

- `tools/ptop` 為唯一主維護目錄
- `libdiag/tools/ptop` 僅保留過渡資訊（或最終移除）
- build/test/docs 全部指向 `tools/ptop`

## 3. 範圍

### In-scope

- Makefile / test script / README / 內部參照路徑調整
- 移除或封存歷史副本的維護入口
- 補一份遷移紀錄文件

### Out-of-scope

- 大規模功能重寫
- 互動式 UI 大改

## 4. 實作步驟

1. 盤點引用來源
   - 搜索 `libdiag/tools/ptop` 字串是否仍被 build/test/doc 使用。
2. 固定唯一入口
   - `make -C tools/ptop standalone`
   - `bash tools/ptop/tests/run_tests.sh`
3. 歷史副本處置（二選一）
   - A. 保留目錄但加醒目 `DEPRECATED.md` + 停止更新
   - B. 直接刪除並在 changelog 記錄（需團隊同意）
4. 文件同步
   - 根 README 與工具 README 都改成 `tools/ptop` 唯一路徑
5. 驗證
   - clean build + tests + smoke

## 5. 風險與緩解

- 風險：課程評分或舊腳本仍指向舊路徑
  - 緩解：保留過渡 stub（例如提示遷移）1 個里程碑週期
- 風險：刪除過快導致回溯困難
  - 緩解：先標註 deprecated，再擇期移除

## 6. 驗收標準

- `rg "libdiag/tools/ptop"` 不再出現在活躍 build/test 路徑
- `tools/ptop` build/test 全綠
- 文件明確聲明唯一來源

## 7. 交付物

- `docs/migration_ptop_single_source.md`（或同等文件）
- 更新後的 Makefile/test/doc

