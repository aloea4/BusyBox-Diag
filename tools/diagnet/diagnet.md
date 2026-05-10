# diagnet — Network Connection Monitor

`diagnet` 是 BusyBox-Diag 系統診斷工具集的一員，從 Linux `/proc/net/tcp` 與 `/proc/net/udp` 讀取即時連線資訊，提供連線列表、狀態分佈統計、可疑監聽 Port 偵測等功能，並支援以表格或 JSON 格式輸出。底層的 `/proc` 解析由共用模組 [libdiag](../../libdiag/) 提供，與同專案的 `diagps`、`diagfs` 共用同一份資料收集邏輯。

---

## Features

- **TCP connection listing** — 解析 `/proc/net/tcp`，顯示本地/遠端 IP:Port 與連線狀態。
- **State distribution stats** — 統計 `ESTABLISHED`、`LISTEN`、`TIME_WAIT` 等各狀態數量。
- **UDP support** — 解析 `/proc/net/udp`，與 TCP 共用同一處理流程。
- **Suspicious port detection** — 比對內建白名單，標記非常見服務 Port 的監聽行為。
- **JSON output** — 結構化輸出方便與其他工具透過 UNIX pipe 串接。

---

## Build

`diagnet` 的編譯流程使用本目錄下獨立的 `Makefile`：

```bash
cd tools/diagnet
make            # 自動 sub-make ../../libdiag 後連結 diagnet
```

編譯產生的執行檔位於 `tools/diagnet/diagnet`。預設使用以下 flag（見 [Makefile](Makefile)）：

```text
gcc -Wall -Wextra -std=c99 -I../../libdiag/include
```

---

## Synopsis

```text
diagnet [--proto tcp|udp|all] [--state STATE] [--stats]
        [--suspicious] [--whitelist PORTS] [--output table|json]
        [-h|--help]
```

---

## Options

| Option | Argument | 說明 |
| --- | --- | --- |
| `--proto` | `tcp` / `udp` / `all` | 過濾協定，預設 `all` |
| `--state` | TCP 狀態名 | 僅顯示指定狀態（如 `ESTABLISHED`、`LISTEN`、`TIME_WAIT`） |
| `--stats` | — | 僅輸出狀態分佈摘要，不列出每條連線 |
| `--suspicious` | — | 僅列出可疑監聽 Port |
| `--whitelist` | `PORT[,PORT...]` | 額外加入白名單的 Port，逗號分隔（例 `8888,9090`） |
| `--output` | `table` / `json` | 輸出格式，預設 `table` |
| `-h`, `--help` | — | 顯示使用說明後結束 |

---

## Output Formats

### Table（預設）

預設輸出對齊的純文字表格，可疑連線於行尾以 `[WARNING]` 標記：

```text
Proto Local Address    Port   Remote Address   Port   State
tcp   127.0.0.1        22     0.0.0.0          0      LISTEN
tcp   192.168.1.5      54321  93.184.216.34    443    ESTABLISHED
tcp   0.0.0.0          8080   0.0.0.0          0      LISTEN        [WARNING]
```

實作細節見 [diagnet.c:199](diagnet.c#L199) 的 `print_table()`。

### JSON（`--output json`）

JSON 物件包含三個欄位：

| Key | 說明 |
| --- | --- |
| `connections` | 通過所有過濾條件後的連線陣列，每筆含 `proto`、`local_addr`、`local_port`、`remote_addr`、`remote_port`、`state`、`suspicious` |
| `summary` | 統計摘要：`total` 與各 TCP 狀態的計數 |
| `suspicious_ports` | 可疑監聽 Port 的精簡列表（僅 `proto`、`local_addr`、`local_port`、`state`） |

範例：

```json
{
  "connections": [
    {"proto": "tcp", "local_addr": "127.0.0.1", "local_port": 22, "remote_addr": "0.0.0.0", "remote_port": 0, "state": "LISTEN", "suspicious": false}
  ],
  "summary": {
    "total": 1,
    "ESTABLISHED": 0,
    "SYN_SENT": 0,
    "SYN_RECV": 0,
    "FIN_WAIT1": 0,
    "FIN_WAIT2": 0,
    "TIME_WAIT": 0,
    "CLOSE": 0,
    "CLOSE_WAIT": 0,
    "LAST_ACK": 0,
    "LISTEN": 1,
    "CLOSING": 0,
    "UNKNOWN": 0
  },
  "suspicious_ports": [
    {"proto": "tcp", "local_addr": "0.0.0.0", "local_port": 8080, "state": "LISTEN"}
  ]
}
```

實作細節見 [diagnet.c:233](diagnet.c#L233) 的 `print_json()`。

---

## Suspicious Port Detection

一條連線會被標記為 `suspicious` 的條件：

1. 連線狀態為 `LISTEN`，且
2. 其 `local_port` 同時不在內建白名單與 `--whitelist` 自訂白名單中。

內建白名單（27 個常見服務 Port，定義於 [diagnet.c:11](diagnet.c#L11) `WHITELIST_DEFAULT[]`）：

```text
20, 21, 22, 23, 25, 53, 67, 68, 69, 80, 110,
123, 143, 161, 162, 389, 443, 465, 514, 587,
636, 993, 995, 3306, 5432, 6379, 27017
```

`--whitelist` 為**加成式**擴充，不會覆蓋內建清單。

---

## Examples

```bash
# 列出所有 TCP + UDP 連線
diagnet

# 僅顯示 TCP
diagnet --proto tcp

# 僅顯示 UDP
diagnet --proto udp

# 狀態分佈摘要（不列出每條連線）
diagnet --stats

# 只顯示已建立的連線
diagnet --state ESTABLISHED

# 列出可疑監聽 Port
diagnet --suspicious

# 把 8888 與 9090 加入白名單
diagnet --whitelist 8888,9090

# 以 JSON 輸出並用 jq 取出 summary
diagnet --output json | jq .summary

# 驗證 JSON 格式合法
diagnet --output json | python3 -m json.tool
```

---

## Exit Codes

| Code  | 意義                                                              |
| ----- | ----------------------------------------------------------------- |
| `0`   | 執行成功，或顯示了 `--help`                                       |
| `1`   | CLI 參數錯誤、`/proc/net/{tcp,udp}` I/O 失敗，或記憶體配置失敗    |

錯誤訊息一律輸出至 `stderr`，正常結果則輸出至 `stdout`，方便 pipe 串接。

---

## Limitations

- **僅支援 Linux**：依賴 `/proc/net/tcp` 與 `/proc/net/udp` 虛擬檔案系統，無法在 Windows / macOS 直接執行。
- **僅 IPv4**：libdiag 目前的 `diag_net.c` 僅解析 IPv4 連線；未支援 `/proc/net/tcp6`、`/proc/net/udp6`。
- **被動偵測**：`--suspicious` 僅根據 Port 號比對白名單，不分析封包內容或行程身分。

---

## Architecture (Brief)

`diagnet` 本身僅負責 CLI 解析、過濾、聚合與輸出格式化；所有底層 `/proc` 讀取與欄位解析皆委由共用模組 [libdiag](../../libdiag/) 處理。主要透過下列 callback API 收集連線資料（定義於 [libdiag/include/libdiag/diag_net.h](../../libdiag/include/libdiag/diag_net.h)）：

```c
int diag_net_foreach_tcp(int (*cb)(const diag_net_conn_t *, void *), void *user);
int diag_net_foreach_udp(int (*cb)(const diag_net_conn_t *, void *), void *user);
```

完整設計（資料流、函式職責、技術方案）採四層架構：CLI 解析層（`getopt_long`）→ 資料收集層（`libdiag` callback）→ 分析處理層（白名單比對、狀態統計）→ 輸出層（table / JSON），詳見本文件各節說明。

---

## See Also

- [libdiag/](../../libdiag/) — 共用資料收集模組，提供 `/proc` 解析的底層實作
- `netstat(8)`, `ss(8)` — GNU / util-linux 對應工具，效能基準測試的對照組
