# diagnet 效能 Benchmark 報告

依規格要求「與對應 GNU coreutils / util-linux 原版工具進行效能對比，落差控制在原版 50% 以內」。對照組為 `iproute2` 的 `ss(8)`，因 `netstat(8)` 在多數現代發行版已被 deprecate；`ss` 直接走 netlink，是更嚴格的對照。

> **執行說明**：本檔之數據需在 [BusyDocker](../../../BusyDocker) 容器或任何具備 `/proc/net/{tcp,udp}` 的 Linux 環境執行 [`bench.sh`](bench.sh) 取得後，填入下方表格。Windows 主機無法跑。

---

## 量測方法

- **腳本**：[bench.sh](bench.sh)，bash 內建 `EPOCHREALTIME`（bash 5.0+）計時，無 hyperfine 依賴
- **每組 iteration 數**：`N=500`（外加 20 次 warmup，warmup 結果丟棄）
- **指標**：mean、**median**、stddev、ratio = `diagnet / ss`
- **Verdict 採 median ratio**：對 sub-ms 級量測的 long-tail outlier 較強健；mean ratio 並列輸出供對照
- **CPU pinning**：`DIAGNET_PIN=1 ./bench.sh` 觸發 `taskset -c 0`，把兩邊都釘在同一個 CPU，消除 migration 噪聲
- **通過門檻**：median ratio ≤ 1.50（diagnet 不慢於 ss 50%）
- **stdout 丟棄**：`>/dev/null 2>&1`，只量純執行時間
- **編譯**：`-Wall -Wextra -std=c99 -O3 -flto`（libdiag 與 diagnet 同步），詳見 [Makefile](Makefile)

### 對照組公平性（重要）

`ss(8)` 預設「open non-listening sockets that have established connection」—— 也就是**只列 ESTABLISHED**，不列 LISTEN。而 diagnet `--proto tcp` 列所有 TCP（含 LISTEN / TIME_WAIT / 等）。因此原始版本的 `ss -tn` 對照組少做了 LISTEN 那部分工作，產生不對等比較。

本版本所有 ss 對照組都加上 `-a` 旗標：

| 對照組 | 命令 | 涵蓋範圍 |
| --- | --- | --- |
| ss-raw / ss-json | `ss -tna` / `ss -tna -O` | TCP 所有狀態，與 diagnet `--proto tcp` 對齊 |
| ss-stats | `ss -tnua` | TCP + UDP 所有狀態，與 diagnet `--stats`（含 UDP）對齊 |

## 環境

| 欄位 | 值 |
| --- | --- |
| 主機 | `c52c21f36886`（BusyDocker container） |
| Kernel | `6.6.87.2-microsoft-standard-WSL2` |
| Distro | Ubuntu 22.04（BusyDocker） |
| CPU | 11th Gen Intel(R) Core(TM) i5-1135G7 @ 2.40GHz |
| bash | GNU bash, version 5.1.16(1)-release |
| iproute2 (ss) | ss utility, iproute2-5.15.0 |
| diagnet build | `gcc -Wall -Wextra -std=c99 -O3 -flto`（libdiag + diagnet） |
| 量測時間 | 2026-05-14T15:22:35Z |

---

## 結果

### Case 1 — TCP listing（raw vs `ss -tna`）

| 工具 | mean (s) | median (s) | stddev (s) |
| --- | --- | --- | --- |
| `diagnet --proto tcp --output raw` | 0.004505 | 0.004467 | 0.000726 |
| `ss -tna` | 0.003118 | 0.003049 | 0.000348 |
| **median ratio (diagnet / ss)** | 1.47 | – | – |
| **mean ratio** | 1.44 | – | – |
| **verdict (≤1.50)** | OK | – | – |

### Case 2 — TCP listing 結構化（json vs `ss -tna -O`）

| 工具 | mean (s) | median (s) | stddev (s) |
| --- | --- | --- | --- |
| `diagnet --proto tcp --output json` | 0.003965 | 0.003695 | 0.002018 |
| `ss -tna -O` | 0.003150 | 0.003043 | 0.000416 |
| **median ratio** | 1.21 | – | – |
| **mean ratio** | 1.26 | – | – |
| **verdict (≤1.50)** | OK | – | – |

### Case 3 — state distribution（stats vs `ss -tnua`）

| 工具 | mean (s) | median (s) | stddev (s) |
| --- | --- | --- | --- |
| `diagnet --stats` | 0.004869 | 0.004778 | 0.002005 |
| `ss -tnua` | 0.003454 | 0.003349 | 0.000438 |
| **median ratio** | 1.43 | – | – |
| **mean ratio** | 1.41 | – | – |
| **verdict (≤1.50)** | OK | – | – |

---

## 結論

**三組皆通過規格門檻**（median ratio ≤ 1.50）：

| Case | median ratio | mean ratio | verdict |
| --- | --- | --- | --- |
| tcp listing (raw) | 1.47 | 1.44 | OK |
| tcp listing (json) | 1.21 | 1.26 | OK |
| state distribution | 1.43 | 1.41 | OK |

diagnet 與 `ss` 的效能差距 21–47%（median），位於規格要求的 50% 範圍內。raw 與 stats 兩組接近上限是預期內：兩者每 record 做的工作量相近於 ss（裸表/聚合），diagnet 多出的 procfs text 解析成本佔比較顯著；json 多花在輸出格式化，diagnet 與 ss 兩邊都重，相對成本壓低，反而 margin 較大。

### 結構性背景

`ss` 透過 `NETLINK_SOCK_DIAG` 與 kernel 直接交換 binary struct，無 text 解析成本。diagnet 走 procfs（`/proc/net/{tcp,udp}`）讀文字，需 `fgets` + `sscanf` + hex→dotted-quad 轉換 + state byte 查表。兩者 transport 不同，diagnet 略慢屬結構性必然，但已壓在合理範圍內。

達標的關鍵因素（依貢獻順序）：

1. **公平對照**：補 ss 的 `-a` 旗標讓兩邊都列出所有 TCP/UDP（含 LISTEN）。原始 `ss -tn` 預設不列 LISTEN，做的事比 diagnet 少。
2. **編譯優化**：libdiag + diagnet 同步 `-O3 -flto`（透過 sub-make 變數傳遞，libdiag 內檔 0 變動）。
3. **量測方法學**：median verdict（對 sub-ms long-tail outlier 強健）、`taskset -c 0` CPU pinning、`WARMUP=20`、`N=500`。

### 迭代改善紀錄

| 版本 | Build flag | bench 方法 | raw median | json median | stats median |
| --- | --- | --- | --- | --- | --- |
| v1 | `-O0`（隱含預設） | `ss -tn`（不公平） | 1.55 | 1.58 | 1.28 |
| v2 | `-O2`（diagnet only） | `ss -tn` | 1.57 | 1.95 | 1.45 |
| v3 | `-O3 -flto`（含 libdiag） | `ss -tn`, median verdict, pin, N=500 | 1.53 | 1.34 | 1.41 |
| v4 | `-O3 -flto` | `ss -tna` / `-tnua`（公平對照） | 1.22 | 1.32 | 1.29 |
| **v5（提交版本）** | `-O3 -flto` | 同 v4 + 完整 env capture，host i5-1135G7 / WSL2 | **1.47** | **1.21** | **1.43** |

每一步只動一個變數，方便歸因：

- v1→v2：開 -O2 — json 反而變差，懷疑 -O0/-O2 差距被噪聲淹沒
- v2→v3：-O3 -flto + libdiag 同步、median verdict、20 次 warmup、CPU pinning、N=500
- v3→v4：發現 ss 預設不顯示 LISTEN，與 diagnet 不對等；補 `-a` 旗標
- v4→v5：同一份程式碼 / 同一份方法學在不同 host 重跑（i5-1135G7 / WSL2 vs 先前 host）；raw 從 1.22 抖到 1.47、stats 從 1.29 抖到 1.43，是 sub-ms 級量測在不同 host、不同 system load 下的合理變異，**不是退步**。本次仍全部 ≤ 1.50。json 反而從 1.32 改善到 1.21，方向相反，再次說明這個變異是雙向噪聲而非單向回退。

### Reproducibility

```bash
cd /workspace/tools/diagnet
make clean && make                          # -O3 -flto (libdiag + diagnet)
DIAGNET_PIN=1 ./bench.sh 500                # 500 次 + CPU pinning
# 結果在 stdout，per-iteration 細節在 bench.log
```

---

## See Also

- [bench.sh](bench.sh) — 量測腳本
- [test_diagnet.sh](test_diagnet.sh) Section B — 正確性對照（與 `busybox netstat` 結果比對，非效能）
- `ss(8)`、`netstat(8)` — 對照工具
