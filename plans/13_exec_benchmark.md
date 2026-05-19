# 執行清單：M3 Benchmark 方法學與 50% 目標

## 任務 1：方法學定版
- 優先級：`P0`
- 預估工時：`1.5h`
- 狀態：`DONE`
- 執行證據：
  - 已建立 `benchmark/benchmark_method.md`。
  - 已修正 ptop 對照命令語意：`top -b -d 1 -n 2`。
- 驗收指令：
```bash
test -f benchmark/benchmark_method.md
```

## 任務 2：對照命令定版
- 優先級：`P0`
- 預估工時：`1.0h`
- 狀態：`DONE`
- 執行證據：
  - 已寫入 `benchmark/run_benchmark.sh`。
- 驗收指令：
```bash
./tools/ptop/ptop --output table -1 -n 1 >/dev/null
./tools/diagfs/diagfs --all --output raw --color never >/dev/null
./tools/diagnet/diagnet --proto tcp --output raw >/dev/null
```

## 任務 3：跑 baseline 並存 raw data
- 優先級：`P1`
- 預估工時：`2.5h`
- 狀態：`DONE`
- 執行證據：
  - 正式樣本已執行：`WARMUP=20 N=200 bash benchmark/run_benchmark.sh`。
  - Raw 檔案：`benchmark/raw_results/benchmark_20260518_082437.csv`。
- 驗收指令：
```bash
ls -lt benchmark/raw_results | head -n 5
```

## 任務 4：產生 final report
- 優先級：`P1`
- 預估工時：`1.5h`
- 狀態：`DONE`
- 執行證據：
  - `benchmark/final_report.md` 已更新為正式樣本結果。
  - 三案例 verdict 皆為 PASS。
- 驗收指令：
```bash
test -f benchmark/final_report.md
sed -n '1,120p' benchmark/final_report.md
```

## 任務 5：超標項優化循環
- 優先級：`P1`
- 預估工時：`2.0h+`
- 狀態：`DONE`
- 收斂結果：
  - 正式樣本（WARMUP=20 / N=200）下：
    - `M3-PTOP-01` ratio = `0.868` → PASS
    - `M3-DIAGFS-01` ratio = `0.920` → PASS
    - `M3-DIAGNET-01` ratio = `1.196` → PASS
  - 所有核心案例皆滿足 `ratio <= 1.50`。
- 驗收標準：
  - 核心案例 `ratio <= 1.50` ✅

