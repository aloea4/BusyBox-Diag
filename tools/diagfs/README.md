# diagfs

## Integration Guide (整合協助)

為了配合 BusyBox-Diag 的單一執行檔架構，本工具的整合方式如下：

### 1. 進入點與參數傳遞
- **函數名稱**：本工具的主函數已從 `main` 更名為 `diagfs_main(int argc, char **argv)`。
- **參數傳遞**：請將外部收到的參數陣列指標，包含工具名稱本身一起傳入。
  - 例如輸入 `./busybox-diag diagfs --help`
  - 預期傳入 `diagfs_main` 的參數應為：`argv[0] = "diagfs"`, `argv[1] = "--help"`。

### 2. Help 說明訊息
- `diagfs_main` 內部已實作參數解析，當偵測到 `argv` 中包含 `--help` 時，會自動印出專屬的 Usage 說明並回傳 `0` (EXIT_OK)。
- 主程式的分發邏輯無需額外為 `diagfs` 撰寫 help 輸出。

### 3. Man Page 安裝
- 本專案目錄下提供標準的手冊檔案 `diagfs.1`。
- 在統一的 `Makefile` 執行 `make install` 時，請協助將此檔案複製到系統的 man 目錄（例如 `/usr/share/man/man1/`），讓使用者可以透過 `man diagfs` 查閱。

### 4. 效能測試
- 透過 `bench.sh` 以中位數做判定，測試結果儲存於 `bench.md`

---

## Diagfs 介紹

`diagfs` 是 BusyBox-Diag 系統診斷工具集中的 **metadata-based Linux filesystem monitoring tool**。

它透過 `libdiag` 封裝 Linux filesystem metadata 來源，取得 filesystem capacity、inode usage，以及選擇性的 FIEMAP extent layout observation。

當使用者指定一個 path 時，`diagfs` 分析的是該 path **所在的 mounted filesystem**，而不是遞迴計算該目錄本身的大小。

```
diagfs /home/project
```

> 查詢的是 `/home/project` 所在掛載點的 filesystem-level metadata，
> 包括 block usage、inode usage 與可選的 FIEMAP extent metadata。
> 行為類似 `df /home/project`，而非 `du /home/project`。

`libdiag` 負責 measurement；`diagfs` 負責 analysis、policy 與 presentation。本工具採用被動式 metadata analysis，不遞迴掃描整個檔案系統，也不將 FIEMAP extent count 過度宣稱為完整 fragmentation benchmark。

---

## Portability Boundary

`diagfs` 採用 POSIX-style CLI 與 C API 設計，但 backend 是 Linux-specific：

| 部分 | 層級 | 說明 |
|---|---|---|
| CLI 行為 | POSIX-style | stdout / stderr、exit code、path-based query |
| `statvfs()` 思路 | POSIX-style | 可攜的 filesystem capacity 模型 |
| `statfs()` | Linux / BSD-like | 可取得 fs type magic，適合 Linux diag |
| `/proc/self/mounts` | Linux-specific | mount namespace aware |
| FIEMAP ioctl | Linux-specific | extent layout metadata |
| color output | Presentation | 不屬於資料層，透過 `--color` 控制 |

---

## Architecture

```
Linux filesystem metadata
        ↓
libdiag  ·····················  Measurement layer
        ↓
diagfs_analyze.c  ············  Analysis layer
        ↓
diagfs_policy.c  ·············  Policy layer
        ↓
diagfs_output.c  ·············  Presentation layer
        ↓
table / raw / json / pipeline
```

### 檔案結構

```
tools/diagfs/
├── diagfs.c              # main / CLI 解析 / flow control
├── diagfs.h              # 所有型別、enum、struct、function prototype
├── diagfs_analyze.c      # Layer 2: 計算百分比、analyze_layout、analyze_filesystem
├── diagfs_policy.c       # Layer 3: threshold 定義、get_default_policy
├── diagfs_output.c       # Layer 4: table / raw / json / color mode
├── diagfs_mount.c        # /proc/self/mounts 掛載表遍歷
└── README.md
```

### Layer 職責邊界

| Layer | 檔案 | 責任 | 不做 |
|---|---|---|---|
| Measurement | `libdiag` | 呼叫 `statfs` / FIEMAP，封裝成 struct | 不判斷健康狀態 |
| Analysis | `diagfs_analyze.c` | 計算使用比例、layout observation | 不決定顏色 |
| Policy | `diagfs_policy.c` | 判斷 OK / WARN / CRITICAL | 不直接 `printf` |
| Presentation | `diagfs_output.c` | table / raw / json / color | 不重新計算資料 |

---

## Build

```bash
make diagfs
```

依賴 `libdiag`，需確保 `libdiag/diag_common.h` 與 `libdiag/diag_fs.h` 可被找到。

---

## Usage

```
diagfs [PATH] [--all] [--real] [--pseudo]
       [--output table|raw|json]
       [--color auto|always|never]
       [--fiemap]
```

### 選項說明

| 選項 | 說明 |
|---|---|
| `PATH` | 指定要分析的路徑（預設為 `/`）。分析的是該路徑所在的 mounted filesystem |
| `--all` | 掃描所有掛載點 |
| `--real` | （搭配 `--all`）只列真實 block-backed filesystem，跳過 pseudo fs（預設行為） |
| `--pseudo` | （搭配 `--all`）包含 proc / sysfs / tmpfs / cgroup 等虛擬檔案系統 |
| `--output table` | 人類可讀的表格輸出（預設） |
| `--output raw` | 空白分隔的單行輸出，適合 shell pipeline |
| `--output json` | JSON 格式輸出，適合程式解析或 `jq` |
| `--color auto` | 根據 `isatty()` 自動決定是否顯示顏色（預設） |
| `--color always` | 強制顯示顏色（例如透過 `less -R` 觀看時） |
| `--color never` | 強制不顯示顏色，適合寫入檔案或 pipe |
| `--fiemap` | 若 filesystem 不支援 FIEMAP，以 exit code 3 明確報錯（而非靜默略過） |
| `--help` | 顯示使用說明 |

---

## Output Formats

### Table（預設，給人看）

```
diagfs - Filesystem Health Checker
------------------------------------------
路徑        : /
檔案系統類型: ext4
[空間使用] 已使用 61440 KB / 總計 102400 KB (60%)
[inode 使用] 8%
[extent 觀察] 數量: 2 (low_extent_count)
------------------------------------------
```

### Raw（給 shell script）

```
/ ext4 102400 61440 40960 60 8 2 ok ok
```

欄位順序：`PATH TYPE TOTAL_KB USED_KB FREE_KB USE% INODE% EXTENT_COUNT SPACE_HEALTH INODE_HEALTH`

### JSON（給程式處理）

```json
{
  "filesystems": [
    {
      "path": "/",
      "type": "ext4",
      "usage": {
        "total_kb": 102400,
        "used_kb": 61440,
        "free_kb": 40960,
        "used_percent": 60
      },
      "inode": {
        "total": 131072,
        "free": 120000,
        "used_percent": 8
      },
      "layout": {
        "fiemap_supported": true,
        "extent_count": 2,
        "observation": "low_extent_count"
      },
      "health": {
        "space": "ok",
        "inode": "ok",
        "layout": "ok"
      },
      "warnings": []
    }
  ]
}
```

---

## Exit Codes

| Code | 意義 |
|---|---|
| `0` | 正常完成 |
| `1` | 執行期錯誤（`statfs` 失敗、mount table 無法讀取） |
| `2` | 參數錯誤（未知選項、缺少引數） |
| `3` | 功能不支援（`--fiemap` 但 filesystem 不支援 FIEMAP） |

---

## Health Policy

預設閾值定義於 `diagfs_policy.c`：

| 指標 | WARN | CRITICAL |
|---|---|---|
| 空間使用率 | ≥ 70% | ≥ 90% |
| inode 使用率 | ≥ 70% | ≥ 90% |
| extent 數量 | > 10 | > 50 |

> inode 使用率採用 **ceiling（無條件進位）** 計算。
> 原因：inode 耗盡會直接導致無法建立新檔案，屬於高風險狀態，保守進位確保提早觸發警告閾值。

---

## FIEMAP Extent Layout Observation

FIEMAP 提供的是**單一檔案的 extent layout metadata**，而非整個 filesystem 的完整 fragmentation score。`diagfs` 以 `extent_count` 作為粗略觀察指標，不宣稱完整的 fragmentation benchmark。

| Observation | 意義 |
|---|---|
| `low_extent_count` | extent 數量較少，推測該檔案配置較為連續 |
| `medium_extent_count` | 數量普通，有輕度分散 |
| `high_extent_count` | 數量偏多，推測檔案實體佈局較分散 |
| `unsupported` | 此 filesystem 不支援 FIEMAP |

---

## Pipeline 範例

```bash
# 找出空間使用率達 critical 的掛載點
diagfs --output raw | awk '$8 == "critical" { print $1 }'

# 找出 inode 達 critical 的掛載點
diagfs --output json | jq '.filesystems[] | select(.health.inode=="critical")'

# 輸出報告至檔案（不含顏色碼）
diagfs --all --output json --color never | tee report.json

# 掃描所有掛載點（含 pseudo filesystem）
diagfs --all --pseudo --output table
```

---

## Benchmark 對照

| diagfs 功能 | 對照工具 |
|---|---|
| disk usage | `df -k` |
| inode usage | `df -i` |
| filesystem type | `df -T` / `stat -f` |
| mount traversal | `findmnt` / `/proc/self/mounts` |
| FIEMAP extent observation | `filefrag -v` |

`diagfs` 的 disk usage 與 inode usage 以 `df -k` / `df -i` 交叉驗證；FIEMAP `extent_count` 則與 `filefrag -v` 的 extent 輸出比對。

---

## Future Work

- `statvfs` backend 提升可攜性
- `--fiemap` 支援 `--all` 模式
- `is_remote` 欄位實作（NFS / CIFS 偵測）
- filesystem-specific policy profile（btrfs / xfs / ext4 各異閾值）
- `DIAGFS_FLAG_READONLY` / `DIAGFS_FLAG_REMOTE` flags 實際填入
- mount namespace awareness report