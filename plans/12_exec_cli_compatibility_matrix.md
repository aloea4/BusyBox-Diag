# 執行清單：M2 CLI 相容矩陣

## 任務 1：建立對照範圍
- 優先級：`P0`
- 預估工時：`1.0h`
- 狀態：`DONE`
- 執行證據：
  - 已確認 `ptop/diagfs/diagnet` help 基線。
- 驗收指令：
```bash
./tools/ptop/ptop --help
./tools/diagfs/diagfs -h
./tools/diagnet/diagnet --help
```
- 完成標準：確定每工具的 reference baseline。

## 任務 2：產生矩陣文件
- 優先級：`P1`
- 預估工時：`2.0h`
- 狀態：`DONE`
- 執行證據：
  - 已建立 `docs/compatibility_matrix.md`（v2），含 L1~L4 與每列 Test Case ID。
- 驗收指令：
```bash
test -f docs/compatibility_matrix.md
rg -n "L1|L2|L3|L4|M2-PTOP-|M2-DIAGFS-|M2-DIAGNET-|M2-EXIT-" docs/compatibility_matrix.md
```

## 任務 3：差異決策與記錄
- 優先級：`P1`
- 預估工時：`1.5h`
- 狀態：`DONE`
- 執行證據：
  - 已建立 `docs/compatibility_decisions.md`。
- 驗收指令：
```bash
test -f docs/compatibility_decisions.md
```

## 任務 4：補 smoke tests
- 優先級：`P0`
- 預估工時：`1.5h`
- 狀態：`DONE`
- 執行證據：
  - `diagfs` / `diagnet` / `ptop` 測試腳本皆可通過（busybox cross-check 允許 SKIP）。
- 驗收指令：
```bash
bash tools/diagfs/test.sh
bash tools/diagnet/test_diagnet.sh
bash tools/ptop/tests/run_tests.sh
```
- 完成標準：至少涵蓋 help/output/usage error。

## 任務 5：help/man/README 一致化
- 優先級：`P1`
- 預估工時：`1.0h`
- 狀態：`DONE`
- 執行證據：
  - README 已新增 `5.4 相容矩陣摘要` 與 `5.5 Test Case ID 導覽`。
  - `docs/compatibility_matrix.md`、`docs/compatibility_decisions.md` 已由 README 直接索引。
  - 已執行 cross-check：

```bash
rg -n "Exit codes|output|no-header|quiet|reserved|compatibility" tools/diagfs tools/diagnet tools/ptop README.md docs
```

