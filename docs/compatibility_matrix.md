# BusyBox-Diag CLI 相容矩陣（Draft v2）

## 分級定義

- **L1 Full**：語法/語意/exit code 與參考工具一致
- **L2 Alias**：語意相近，以別名或不同旗標提供
- **L3 Partial**：主用途相近，但有明顯行為差異
- **L4 Custom**：專題自訂，無直接對應

## Test Case ID 命名

- `M2-PTOP-XX`：ptop 相容項
- `M2-DIAGFS-XX`：diagfs 相容項
- `M2-DIAGNET-XX`：diagnet 相容項
- `M2-EXIT-XX`：跨工具 exit code 契約

---

## A. ptop vs top/toybox top

| BusyBox-Diag (`ptop`) | Reference | Level | 備註 | Test Case ID |
|---|---|---|---|---|
| `-h`, `--help` | `top -h/--help` | L2 | 語法存在，內容非完全同構 | `M2-PTOP-01` |
| `--output table` | `top -b` | L2 | 以 alias 對齊 batch 語意 | `M2-PTOP-02` |
| `--output raw` | 無直接標準 | L4 | 自訂機器可讀輸出 | `M2-PTOP-03` |
| `--output json` | 無直接標準 | L4 | 自訂 JSON | `M2-PTOP-04` |
| `-p PID` | `top -p PID` | L1 | 核心語意一致 | `M2-PTOP-05` |
| `-n COUNT` | `top -n` | L1 | 回圈次數語意一致 | `M2-PTOP-06` |
| `-d SEC` | `top -d` | L1 | 刷新間隔語意一致 | `M2-PTOP-07` |
| `-s pid|cpu|rss` | top 排序選項 | L3 | 介面簡化，不完全同構 | `M2-PTOP-08` |
| `-S STATE` | 無直接標準 | L4 | 自訂狀態過濾 | `M2-PTOP-09` |
| `--no-header` | top 不常見 | L4 | 目前未提供 | `M2-PTOP-10` |
| `--quiet` | top 不常見 | L4 | 目前未提供 | `M2-PTOP-11` |

---

## B. diagfs vs df/stat/filefrag

| BusyBox-Diag (`diagfs`) | Reference | Level | 備註 | Test Case ID |
|---|---|---|---|---|
| `-h`, `--help` | `df --help` | L2 | 主題一致 | `M2-DIAGFS-01` |
| `--output table|raw|json` | `df`（非 json） | L3 | raw/json 為擴充 | `M2-DIAGFS-02` |
| `--all` | `df -a` | L1 | 掃描全部掛載點 | `M2-DIAGFS-03` |
| `--real` / `--pseudo` | `df -x/-t` 組合概念 | L3 | 用法不同但目的相近 | `M2-DIAGFS-04` |
| `--fiemap` | `filefrag` | L3 | 觀察 extent，非完整碎片分數 | `M2-DIAGFS-05` |
| `--no-header` | `df --output` 類似場景 | L3 | 僅影響本工具 banner | `M2-DIAGFS-06` |
| `--quiet` | 無標準對應 | L4 | 自訂 warning 抑制 | `M2-DIAGFS-07` |
| `--color auto/always/never` | GNU 常見模式 | L2 | 模式語意接近 | `M2-DIAGFS-08` |

---

## C. diagnet vs ss/netstat

| BusyBox-Diag (`diagnet`) | Reference | Level | 備註 | Test Case ID |
|---|---|---|---|---|
| `-h`, `--help` | `ss --help` | L2 | 介面同族 | `M2-DIAGNET-01` |
| `--proto tcp|udp|all` | `ss -t/-u/-a` | L3 | 類似但語法不同 | `M2-DIAGNET-02` |
| `--state STATE` | `ss state ...` | L2 | 語意接近 | `M2-DIAGNET-03` |
| `--listen` | `ss -l` | L1 | 常見語意一致 | `M2-DIAGNET-04` |
| `--local-port`, `--remote-port` | `ss sport/dport` | L2 | 語法不同、語意近 | `M2-DIAGNET-05` |
| `--local-addr`, `--remote-addr` | `ss src/dst` | L2 | 語法不同、語意近 | `M2-DIAGNET-06` |
| `--output table|raw|json` | `ss`（無 json） | L3 | json 為擴充 | `M2-DIAGNET-07` |
| `--no-header` | `ss --no-header` | L1 | 一致 | `M2-DIAGNET-08` |
| `--stats` | `ss -s` | L2 | 都是摘要概念 | `M2-DIAGNET-09` |
| `--quiet` | 無標準對應 | L4 | 自訂 warning 抑制 | `M2-DIAGNET-10` |
| `--suspicious`, `--whitelist` | 無標準對應 | L4 | 專題自訂 heuristic | `M2-DIAGNET-11` |

---

## D. Exit Code 契約

| Tool | 0 | 1 | 2 | 3 | Test Case ID |
|---|---|---|---|---|---|
| `ptop` | success/help | runtime error | usage error | 未使用 | `M2-EXIT-01` |
| `diagfs` | success/help | runtime error | usage error | unsupported | `M2-EXIT-02` |
| `diagnet` | success/help | runtime error | usage error | reserved (unused) | `M2-EXIT-03` |

