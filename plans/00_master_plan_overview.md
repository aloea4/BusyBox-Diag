# BusyBox-Diag 專題補強計畫總覽（草案）

本文件為總覽索引，對應四份子計畫：

1. [01_ptop_single_source_consolidation.md](./01_ptop_single_source_consolidation.md)
2. [02_cli_compatibility_matrix_plan.md](./02_cli_compatibility_matrix_plan.md)
3. [03_benchmark_methodology_and_targets.md](./03_benchmark_methodology_and_targets.md)
4. [04_open_source_release_and_docs_plan.md](./04_open_source_release_and_docs_plan.md)

---

## A. 目前狀態摘要

- 三工具與 `libdiag` 已可建置，主要 smoke/test 可跑。
- `tools/ptop` 與 `libdiag/tools/ptop` 並存，存在維護分叉風險。
- CLI 一致性已有基礎，但尚未形成正式相容矩陣與差異治理流程。
- Benchmark 有零散成果，尚缺統一驗收規格（統計口徑 + 門檻判定總表）。
- 文件已大幅補強，但開源交付所需的 `CONTRIBUTING`、發布流程與治理項目仍需系統化。

## B. 建議執行順序（里程碑）

1. **M1：來源收斂**（先消除雙份 `ptop` 風險）
2. **M2：CLI 相容矩陣**（明確定義與 Toybox/GNU 的對照）
3. **M3：Benchmark 驗收框架**（統一方法學與結果門檻）
4. **M4：開源發布與文件封裝**（README / CONTRIBUTING / man / release）

## C. 驗收輸出清單（最終）

- 一份「單一來源樹」與遷移說明
- 一份「CLI 相容矩陣」與差異決策表
- 一份「Benchmark 最終報告」（含 50% 門檻判定）
- 一份「開源發布包」：README、CONTRIBUTING、man pages、版本標記流程

