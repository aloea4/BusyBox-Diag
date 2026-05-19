# Benchmark 結論段落（可直接貼報告）

本專題以統一方法學進行效能驗證，採用 `WARMUP=20`、`N=200`，並以 median 作為主要比較指標。評估公式為：

```text
ratio = custom_median / reference_median
```

驗收門檻設定為 `ratio <= 1.50`（即與參考工具效能差距在 50% 以內）。

正式樣本結果如下：

- `M3-PTOP-01`：custom `1.015581s`、reference `1.169817s`、ratio `0.868`（PASS）
- `M3-DIAGFS-01`：custom `0.009424s`、reference `0.010238s`、ratio `0.920`（PASS）
- `M3-DIAGNET-01`：custom `0.010910s`、reference `0.009126s`、ratio `1.196`（PASS）

整體而言，三個工具在本次正式樣本下皆達成專題效能目標（差距 ≤ 50%）。其中 `ptop` 與 `diagfs` 的 ratio 均低於 1，顯示在對應測試場景下 custom implementation 並未落後於參考命令；`diagnet` 雖高於 1，但仍明確落在可接受門檻內。基於上述結果，本專題在 benchmark 驗收條件上判定為通過。

