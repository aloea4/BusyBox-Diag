# 執行清單：M1 `ptop` 單一來源收斂

## 任務 1：盤點舊路徑參照
- 優先級：`P0`
- 預估工時：`1.0h`
- 狀態：`TODO`
- 執行步驟：
  1. 搜索所有 `libdiag/tools/ptop` 參照。
  2. 分類為 build/test/doc/runtime。
- 驗收指令：
```bash
rg -n "libdiag/tools/ptop" .
```
- 完成標準：有一份分類清單（可貼在 PR 描述）。

## 任務 2：固定唯一入口
- 優先級：`P0`
- 預估工時：`1.0h`
- 狀態：`TODO`
- 執行步驟：
  1. 確保 `tools/ptop/Makefile` 為唯一標準入口。
  2. 驗證 `standalone` 與 tests 全走 `tools/ptop`。
- 驗收指令：
```bash
make -C tools/ptop standalone
bash tools/ptop/tests/run_tests.sh
```
- 完成標準：命令成功且不需進入 `libdiag/tools/ptop`。

## 任務 3：歷史副本處置
- 優先級：`P1`
- 預估工時：`1.5h`
- 狀態：`TODO`
- 執行步驟：
  1. 選擇 A（deprecated）或 B（移除）。
  2. A: 新增 deprecated 提示；B: 移除並在變更記錄說明。
- 驗收指令：
```bash
rg -n "deprecated|DEPRECATED|tools/ptop" libdiag/tools/ptop README.md
```
- 完成標準：團隊可一眼看出主用來源。

## 任務 4：文件同步
- 優先級：`P1`
- 預估工時：`1.5h`
- 狀態：`TODO`
- 執行步驟：
  1. root README、ptop README、相關 man/說明同步。
- 驗收指令：
```bash
rg -n "tools/ptop|libdiag/tools/ptop" README.md tools/ptop/README.md
```
- 完成標準：文件不再指向錯誤主路徑。

## 任務 5：回歸驗證
- 優先級：`P0`
- 預估工時：`0.5h`
- 狀態：`TODO`
- 驗收指令：
```bash
make -C libdiag
make -C tools/ptop standalone
bash tools/ptop/tests/run_tests.sh
```
- 完成標準：全綠。

