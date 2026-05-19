# Benchmark 方法學（M3）

## 1. 驗收目標

對每個案例計算：

```text
ratio = custom_median / reference_median
```

通過條件：

```text
ratio <= 1.50
```

## 2. 環境紀錄

- OS / kernel：`uname -a`（執行腳本會輸出）
- CPU：`/proc/cpuinfo` 摘要
- 執行模式：目前以同機重複測量，未強制 pinning（可擴充）

## 3. 測試流程

1. Warmup：預設 5 次（正式可提高到 20）
2. Sample：預設 30 次（正式可提高到 200~500）
3. 指標：median（主要）、mean（輔助）
4. 每個案例均記錄 custom/ref 原始秒數到 CSV（納秒差值換算）

## 4. 對照命令（v1）

### ptop
- custom：`./tools/ptop/ptop --output table -1 -n 1`
- ref：`top -b -d 1 -n 2`

### diagfs
- custom：`./tools/diagfs/diagfs --all --output raw --color never`
- ref：`df -k`

### diagnet
- custom：`./tools/diagnet/diagnet --proto tcp --output raw`
- ref：`ss -tna`

## 5. 注意事項

- `diagfs` 功能涵蓋 inode / fiemap，`df -k` 僅對齊容量檢視面向。
- `diagnet` 與 `ss` 後端機制不同（procfs vs netlink），結果作相對效能參考。

