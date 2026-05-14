# diagnet — Procfs-based Linux Network Connection Monitoring Tool

`diagnet` 是 BusyBox-Diag 系統診斷工具集中的 **procfs-based Linux network connection monitoring tool**。它透過 Linux `procfs` 匯出的 TCP/UDP connection metadata（`/proc/net/tcp` 與 `/proc/net/udp`）觀察目前系統中的 network connection state，提供 connection listing、TCP state summary、listening port observation、heuristic suspicious port observation，並支援 table / raw / JSON 三種輸出與 shell pipeline 串接。

底層 `/proc` 解析由共用模組 [libdiag](../../libdiag/) 提供 measurement layer；`diagnet` 本身負責 connection analysis、filtering、policy 與 presentation。

`diagnet` 不主動探測遠端主機，也不擷取封包，也不宣稱進行入侵偵測。其 `--suspicious` 功能僅代表基於啟發式規則的可疑監聽 port 觀察（heuristic-based suspicious listening port observation）。

---

## Architecture (Four Layers)

```text
Layer 1: Measurement   libdiag — 讀 /proc/net/{tcp,udp}、解析欄位、封裝 diag_net_conn_t
Layer 2: Analysis      diagnet_analyze.c / diagnet_collect.c — 計數、histogram、串接 filter
Layer 3: Policy        diagnet_policy.c — heuristic flags（suspicious_port / public_listen / many_*）
Layer 4: Presentation  diagnet_output.c — table / raw / json 三種輸出
```

`diagnet.c` 僅做 `main()`、`getopt_long` 與流程控制；CLI 旗標統一在此解析。

---

## Portability Boundary

`diagnet` 採用 POSIX-style command-line design，但 backend 為 Linux-specific。

| Portable / POSIX-style | Linux-specific backend |
| --- | --- |
| stdout / stderr separation | `/proc/net/tcp` |
| stable exit code | `/proc/net/udp` |
| table / raw / JSON output | Linux procfs connection metadata |
| shell pipeline integration | Linux TCP state representation |

也就是：介面是 UNIX-style，資料來源是 Linux procfs。

---

## Build

```bash
cd tools/diagnet
make            # 自動 sub-make ../../libdiag 後連結 diagnet
```

預設 flag（見 [Makefile](Makefile)）：

```text
gcc -Wall -Wextra -std=c99 -I../../libdiag/include
```

---

## Synopsis

```text
diagnet [--proto tcp|udp|all] [--state STATE | --listen]
        [--local-port PORT] [--remote-port PORT]
        [--local-addr ADDR] [--remote-addr ADDR]
        [--suspicious] [--whitelist PORTS]
        [--sort state|local-port|remote-port]
        [--output table|raw|json] [--no-header]
        [--stats] [--quiet] [-h|--help]
```

---

## Options

| Option | Argument | 說明 |
| --- | --- | --- |
| `--proto` | `tcp` / `udp` / `all` | 過濾協定，預設 `all` |
| `--state` | TCP state name | 僅顯示指定狀態；無效值 → exit 2 |
| `--listen` | — | `--state LISTEN` 的捷徑；與 `--state` 互斥 |
| `--local-port` | `1..65535` | 過濾本地 port |
| `--remote-port` | `1..65535` | 過濾遠端 port |
| `--local-addr` | dotted IPv4 | 字串相等比對本地位址 |
| `--remote-addr` | dotted IPv4 | 字串相等比對遠端位址 |
| `--suspicious` | — | 僅列出 heuristic flag 命中的監聽 port |
| `--whitelist` | `PORT[,PORT...]` | 額外加入白名單，逗號分隔 |
| `--sort` | `state` / `local-port` / `remote-port` | 結果排序 |
| `--output` | `table` / `raw` / `json` | 輸出格式，預設 `table` |
| `--no-header` | — | 跳過 table / raw 的標題列 |
| `--stats` | — | 僅輸出 state distribution（streaming，不保存 records） |
| `--quiet` | — | 抑制 stderr 的全域 warning |
| `-h`, `--help` | — | 顯示使用說明後結束 |

---

## Output Formats

### Table（預設）

```text
PROTO LOCAL_ADDR        LPORT  REMOTE_ADDR      RPORT  STATE        FLAGS
tcp   127.0.0.1         22     0.0.0.0          0      LISTEN       -
tcp   0.0.0.0           8080   0.0.0.0          0      LISTEN       suspicious_port,public_listen
udp   0.0.0.0           53     0.0.0.0          0      -            -
```

UDP 的 STATE 一律顯示 `-`；多重 flag 以逗號連接，無 flag 時顯示 `-`。`--no-header` 可跳過首列。

### Raw（`--output raw`）

同欄位、空白分隔、無對齊、無 header，適合 awk / cut：

```text
tcp 0.0.0.0 22 0.0.0.0 0 LISTEN -
tcp 0.0.0.0 8080 0.0.0.0 0 LISTEN suspicious_port,public_listen
udp 0.0.0.0 53 0.0.0.0 0 - -
```

```bash
diagnet --output raw | awk '$6=="LISTEN" {print $2 ":" $3}'
```

### JSON（`--output json`）

```json
{
  "connections": [
    {
      "protocol": "tcp",
      "local_address": "0.0.0.0",
      "local_port": 22,
      "remote_address": "0.0.0.0",
      "remote_port": 0,
      "state": "LISTEN",
      "flags": ["suspicious_port", "public_listen"]
    }
  ],
  "summary": {
    "total": 1,
    "tcp": 1,
    "udp": 0,
    "states": {
      "LISTEN": 1
    }
  },
  "warnings": [
    { "type": "many_time_wait", "message": "many TIME_WAIT sockets observed" }
  ]
}
```

注意：
- 欄位採 `protocol` / `local_address` / `remote_address`（不是 `proto` / `local_addr`）。
- UDP `state` 一律 `"NONE"`。
- `summary.states` 只列出實際出現過的狀態。
- `summary.tcp + summary.udp == summary.total`。
- 系統級的觀察（如 TIME_WAIT 過多）會列在頂層 `warnings`，而非每筆 connection 的 flag。

```bash
diagnet --output json | jq '.summary.states'
diagnet --output json | jq '.connections[] | select(.state=="ESTABLISHED")'
```

---

## Heuristic-based Suspicious Listening Port Observation

`diagnet` 提供啟發式可疑監聽 port 觀察，並**不**進行入侵偵測、惡意連線判讀或安全掃描。可能掛載在 `connections[].flags` 的旗標：

| Flag | 條件 |
| --- | --- |
| `suspicious_port` | `state == LISTEN` 且 `local_port` 不在內建白名單 + `--whitelist` 加成 |
| `public_listen` | `state == LISTEN` 且 `local_address == 0.0.0.0` |

內建白名單（27 個常見服務 port，定義於 [diagnet_policy.c](diagnet_policy.c)）：

```text
20, 21, 22, 23, 25, 53, 67, 68, 69, 80, 110,
123, 143, 161, 162, 389, 443, 465, 514, 587,
636, 993, 995, 3306, 5432, 6379, 27017
```

`--whitelist` 為加成式擴充，不會覆蓋內建清單。

頂層 `warnings`（聚合觀察，輸出到 stderr 或 JSON `warnings` 陣列；`--quiet` 抑制 stderr）：

| Type | 條件 |
| --- | --- |
| `many_time_wait` | TIME_WAIT 計數 ≥ 50 |
| `many_close_wait` | CLOSE_WAIT 計數 ≥ 50 |

---

## Exit Codes

| Code | 意義 |
| --- | --- |
| `0` | 成功，或顯示了 `--help` |
| `1` | runtime error：`/proc/net/{tcp,udp}` 讀取失敗、記憶體配置失敗 |
| `2` | usage error：未知 flag、`--state INVALID`、`--whitelist abc`、`--output xml`、`--listen` 與 `--state` 同時出現 |
| `3` | unsupported：保留給 future feature（目前未主動回傳） |

正常輸出 → stdout；錯誤、warning → stderr。

---

## Memory Behavior

- 一般 listing 模式：保留每筆通過 filter 的 record 以便排序與輸出。
- `--stats` 模式：採 streaming aggregation，**不**保存 record 陣列，只累計 `tcp_total / udp_total / state_counts / suspicious_count`，適合 BusyBox / embedded 場景。

---

## Examples

```bash
diagnet                                                    # 預設 table
diagnet --proto tcp --sort state                           # 依 state 排序的 TCP
diagnet --listen --no-header                               # listening sockets，無標題
diagnet --local-port 22 --output raw                       # 與 awk 串接
diagnet --suspicious --whitelist 8888,9090                 # 自訂白名單
diagnet --stats                                            # streaming 摘要
diagnet --output json | python3 -m json.tool               # JSON 驗證
diagnet --output json | jq '.warnings'                     # 觀察系統級警告
```

---

## Benchmark / Cross-check

`diagnet` 的 TCP/UDP listing 與 state summary 可與 `ss` / `netstat` 結果交叉驗證：

| diagnet 功能 | 對照工具 |
| --- | --- |
| TCP/UDP listing | `ss -tun` |
| listening ports | `ss -ltnu` |
| TCP state | `ss -ant` |
| legacy 對照 | `netstat -ant` |
| state summary | `ss -ant state ...` 或自行統計 |

`tools/diagnet/test_diagnet.sh` 內含 BusyBox `netstat` cross-check；效能對比腳本見 [bench.sh](bench.sh) 與報告 [bench.md](bench.md)。

---

## Compatibility with `ss` / `netstat` / Toybox

diagnet 採 GNU `ss` / Toybox 的 long-option 慣例（`--proto`、`--listen`、`--state`），讓熟悉這些工具的使用者可以零學習成本上手。下表列出常見操作的對應命令與差異：

| diagnet flag | 等效 `ss` 命令 | 等效 `netstat` 命令 | 等效 Toybox `netstat` | 差異說明 |
| --- | --- | --- | --- | --- |
| `--proto tcp` | `ss -t` | `netstat -t` | `netstat -t` | 一致 |
| `--proto udp` | `ss -u` | `netstat -u` | `netstat -u` | 一致 |
| `--listen` | `ss -l` | `netstat -l` | `netstat -l` | 一致 |
| `--state LISTEN` | `ss state listening` | `netstat \| grep LISTEN` | （需 grep） | diagnet 在 CLI 直接過濾 |
| `--state ESTABLISHED` | `ss state established` | `netstat \| grep ESTAB` | （需 grep） | 同上 |
| `--local-port N` | `ss sport = :N` | （需 grep） | （需 grep） | diagnet 內建 port filter |
| `--remote-port N` | `ss dport = :N` | （需 grep） | （需 grep） | 同上 |
| `--local-addr ADDR` | `ss src ADDR` | （需 grep） | （需 grep） | 字串相等比對 |
| `--remote-addr ADDR` | `ss dst ADDR` | （需 grep） | （需 grep） | 同上 |
| `--output table`（預設） | `ss -tunal` 預設輸出 | `netstat -tunal` 預設輸出 | 同 | 一致；diagnet 多 `FLAGS` 欄 |
| `--output raw` | `ss --no-header` 接近 | – | – | diagnet 欄位固定無對齊，方便 `awk`/`cut` |
| `--output json` | `ss -O`（非 JSON） | （不支援） | （不支援） | diagnet 提供 stable JSON schema |
| `--no-header` | `ss --no-header` | – | – | 一致 |
| `--sort state` | `ss -A inet`（內排序） | – | – | diagnet 額外 |
| `--stats` | `ss -ant \| awk` 自行統計 | `netstat -s` 不同語意 | – | diagnet 提供 state distribution + streaming 不存記憶體 |
| `--suspicious` | （無對應） | （無對應） | （無對應） | diagnet 新增的 heuristic 觀察 |
| `--help` | `ss --help` | `netstat --help` | `netstat --help` | 一致 |
| Exit code | 0 / 非 0 | 0 / 非 0 | 0 / 非 0 | diagnet 多區分 usage error = 2、unsupported = 3 |

### 設計原則

- **介面層相容**：與 `ss` 等對應工具學習成本一致；常見查詢可一對一替換。
- **資料層獨立**：diagnet 走 procfs（`/proc/net/{tcp,udp}`）解析；`ss` 走 netlink。兩者結果可交叉驗證但 transport 不同。
- **輸出層擴充**：保留 `ss` 沒有的 stable `json` schema、`raw` 對齊－free 輸出，方便寫腳本與測試。
- **語意正確性優於相容性**：UDP 沒有 TCP state machine，因此 diagnet 對 UDP record 顯示 `state = "-"`（table）或 `"NONE"`（JSON）；不沿用 `ss` 將 UDP 也標 `UNCONN` 的慣例，避免使用者誤把 datagram socket 套上 connection state 解讀。

---

## Future Work (P2)

- IPv6：`/proc/net/tcp6`、`/proc/net/udp6`
- namespace-aware view（網路 namespace）
- process ownership mapping（PID / comm）
- cgroup / container 分組
- optional service name resolution

這些目前 **沒有** 實作；CLI 也未開啟對應 flag。

---

## See Also

- [libdiag/](../../libdiag/) — 共用 measurement layer
- `ss(8)`, `netstat(8)` — 對照工具
