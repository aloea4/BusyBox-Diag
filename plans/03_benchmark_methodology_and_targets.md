# 計畫 3：Benchmark 方法學與 50% 差距目標（草案）

## 1. 目標

對三工具建立統一 benchmark 方法，並完成「相對原版工具效能差距 ≤ 50%」驗收證據。

## 2. 驗收指標定義

令：

```text
ratio = custom_tool_median_time / reference_tool_median_time
```

通過條件：

```text
ratio <= 1.50
```

## 3. 測試方法學（統一規格）

- 固定資料面：明確標註 kernel 版本、CPU、記憶體、容器/裸機
- Warmup：至少 20 次
- 正式樣本：至少 200~500 次
- 統計：median 為主，mean/std 作為輔助
- CPU pinning：可行時固定核心
- 干擾控制：背景工作最小化

## 4. 對照命令建議

### ptop
- custom：`./tools/ptop/ptop --output table -1 -n 1`
- ref：`top -b -n 1`（或 toybox `top -b -n 1`）

### diagfs
- custom：`./tools/diagfs/diagfs --all --output raw --color never`
- ref：`df -k` + `df -i`（必要時 filefrag 單獨對照）

### diagnet
- custom：`./tools/diagnet/diagnet --proto tcp --output raw`
- ref：`ss -tna`

## 5. 必要輸出文件

1. `benchmark/benchmark_method.md`
2. `benchmark/raw_results/*.csv`
3. `benchmark/final_report.md`
   - 每案例 ratio
   - 是否 <=1.50
   - 問題案例與優化建議

## 6. 優化策略（若超標）

- 減少重複 parse/alloc
- 降低輸出格式化成本（先收斂 raw）
- 熱路徑函式 inline/資料結構簡化
- 避免不必要字串轉換

## 7. 驗收標準

- 三工具皆有可重現 benchmark 腳本
- 報告可由第三方重跑與驗證
- 所有核心案例 ratio ≤ 1.50，或具備差異說明與補救計畫

