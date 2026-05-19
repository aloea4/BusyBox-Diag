# BusyBox-Diag 可執行任務總表（Execution Backlog）

## 使用方式

- 狀態欄位建議：`TODO` / `DOING` / `DONE` / `BLOCKED`
- 優先級：`P0`（阻塞） > `P1`（高） > `P2`（中）

---

## 里程碑 M1：ptop 單一來源收斂

對應文件：`plans/11_exec_ptop_single_source.md`

1. `P0` 盤點 `libdiag/tools/ptop` 參照點（1.0h）
2. `P0` 凍結 `tools/ptop` 唯一 build/test 入口（1.0h）
3. `P1` 歷史副本 deprecated 或移除策略落地（1.5h）
4. `P1` README/man/測試路徑同步（1.5h）
5. `P0` 回歸驗證（0.5h）

估計總工時：`5.5h`

---

## 里程碑 M2：CLI 相容矩陣

對應文件：`plans/12_exec_cli_compatibility_matrix.md`

1. `P0` 建立工具-參考工具對照表（1.0h）
2. `P1` 產生完整選項矩陣（2.0h）
3. `P1` 差異決策（補齊/保留）與文件化（1.5h）
4. `P0` 補齊關鍵 smoke 測試（1.5h）
5. `P1` help/man/README 一致化（1.0h）

估計總工時：`7.0h`

---

## 里程碑 M3：Benchmark 驗收框架

對應文件：`plans/13_exec_benchmark.md`

1. `P0` 固化方法學（環境、warmup、樣本、統計）（1.5h）
2. `P0` 三工具對照命令定版（1.0h）
3. `P1` 跑 baseline + 匯出 raw data（2.5h）
4. `P1` 產出 final ratio 報告（1.5h）
5. `P1` 超標案例優化循環（2.0h+）

估計總工時：`8.5h`（不含大量優化反覆）

---

## 里程碑 M4：開源發布封裝

對應文件：`plans/14_exec_open_source_release.md`

1. `P0` 新增 `CONTRIBUTING.md`（1.0h）
2. `P1` README/man 最終校稿（1.5h）
3. `P1` release checklist + version tag 流程（1.0h）
4. `P1` CI 最小流程（build+tests）（2.0h）
5. `P2` 額外治理檔（LICENSE/CODE_OF_CONDUCT）（1.0h）

估計總工時：`6.5h`

---

## 建議執行順序

`M1 -> M2 -> M3 -> M4`

