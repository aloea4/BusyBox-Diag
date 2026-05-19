# BusyBox-Diag 使用說明書

## 1. 專案總覽

BusyBox-Diag 是一套整合進 BusyBox 的輕量 Linux 診斷工具組。
它會新增三個 BusyBox applet：

- `diagfs`
- `diagnet`
- `ptop`

並共用同一層 `libdiag` measurement layer。

請注意：最終交付目標不是三個彼此獨立的 standalone 程式，而是 BusyBox applet integration。也就是三個工具最後都會被編進同一個 `busybox` binary，透過 BusyBox 的 applet dispatch 機制執行。

## 2. 開始前所需環境

必要環境：

- Linux 主機或 Linux 容器
- 編譯工具鏈（`gcc`、`make`、build essentials）
- `python3`（JSON 驗證用）
- `wget`、`tar`
- BusyBox 1.36.1 source tree
- BusyBox-Diag `final_project` repository

Ubuntu 22.04 參考安裝：

```bash
apt update
apt install -y build-essential make gcc libncurses-dev bison flex python3 wget tar
```

## 3. 下載 BusyBox 原始碼

```bash
mkdir -p ~/workspace
cd ~/workspace

wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2
```

`busybox-1.36.1` 是上游 BusyBox source tree；BusyBox-Diag 會以 overlay 套在這個 tree 上。

驗證：

```bash
ls ~/workspace/busybox-1.36.1
```

預期至少看到：

- `applets/`
- `include/`
- `miscutils/`
- `libbb/`
- `Makefile`
- `Config.in`

重要目錄說明：

- `applets/`：applet 註冊/dispatch 相關 metadata
- `include/`：BusyBox 共用標頭
- `miscutils/`：本專題 applet 的主要整合位置
- `libbb/`：BusyBox 內部共用函式庫
- `Config.in`：Kconfig 入口

## 4. 取得 BusyBox-Diag Repo

```bash
cd ~/workspace
git clone <YOUR_REPO_URL> final_project
```

若不使用 git clone，也可直接把專案放到：

```text
~/workspace/final_project
```

最終目錄應為：

```text
~/workspace/
├── busybox-1.36.1/
└── final_project/
```

## 5. 一鍵 Bootstrap 安裝（建議主路徑）

這是最推薦的使用方式。

```bash
cd ~/workspace/final_project
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1
```

若不想讓腳本自動修改 `~/.bashrc`：

```bash
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1 --no-activate
```

bootstrap 會做的事：

1. 套用 BusyBox-Diag overlay 到 BusyBox source tree
2. 同步 helper scripts 到 BusyBox tree（`build/verify/install`）
3. 編譯 BusyBox（含 `diagfs/diagnet/ptop`）
4. 執行驗證
5. 安裝 `diagfs/diagnet/ptop` 到 `/usr/local/busybox-diag/bin`
6. 視選項決定是否更新 PATH 設定

## 6. 安裝後驗證

如果你使用 `--no-activate`，請先手動設定 PATH：

```bash
export PATH=/usr/local/busybox-diag/bin:$PATH
```

檢查路徑與 help：

```bash
which diagfs
which diagnet
which ptop

diagfs --help
diagnet --help
ptop --help
```

預期：

- `which diagfs` 指向 `/usr/local/busybox-diag/bin/diagfs`
- 三個 `--help` 都能正常輸出

## 7. 基本使用

### 7.1 diagfs

```bash
diagfs --output table /
diagfs --output raw /
diagfs --output json /
```

輸出模式：

- `table`：人類閱讀
- `raw`：shell pipeline 友善
- `json`：自動化/程式解析

常見欄位意義：

- `Path`：檢查路徑
- `Filesystem type`：檔案系統類型
- `Space usage`：空間使用量
- `Inode usage`：inode 使用率
- `Extent count`：extent 觀察值

若出現 `Filesystem type: unknown`：

某些容器檔案系統（例如 overlay 類型）可能以輕量偵測器未涵蓋的形式回報 type。這不一定代表失敗；空間與 inode 指標通常仍有效。

### 7.2 diagnet

```bash
diagnet --output table
diagnet --output raw
diagnet --output json
```

`diagnet` 是 **Passive kernel socket state observer**。
它讀取 `/proc/net/tcp` 與 `/proc/net/udp`。

它不會：

- 主動建立 socket
- 探測遠端主機
- 抓封包

常見輸出欄位：

- protocol
- local address / local port
- remote address / remote port
- TCP state
- warning/flag（若有）

### 7.3 ptop

```bash
ptop --output table -1 -n 1
ptop --output raw -1 -n 5
ptop --output json -1 -n 1
```

說明：

- `-1`：one-shot 模式
- `-n N`：更新次數
- `-t N`：顯示前 N 個 process（若使用）
- `ptop` 是 snapshot-based 輕量監控器，不是完整 ncurses top clone

## 8. JSON 驗證

```bash
diagfs --output json / | python3 -m json.tool
diagnet --output json | python3 -m json.tool
ptop --output json -1 -n 1 | python3 -m json.tool
```

若以上都通過，表示 JSON 語法合法且可被機器解析。

## 9. Pipeline 範例

```bash
diagfs --output raw / | awk '{print $1, $2, $3}'
diagnet --output raw | head -20
ptop --output raw -1 -n 10 | head -20
```

這些範例用來驗證 UNIX pipeline / script 友善性。

## 10. 手動流程（不走 bootstrap）

```bash
cd ~/workspace/final_project
scripts/apply_overlay.sh ~/workspace/busybox-1.36.1

cd ~/workspace/busybox-1.36.1
scripts/build_busybox_diag.sh
scripts/verify_busybox_diag.sh
scripts/install_busybox_diag_local.sh --activate
```

各腳本用途：

- `apply_overlay.sh`：套 overlay
- `build_busybox_diag.sh`：可重現編譯 + smoke
- `verify_busybox_diag.sh`：驗證矩陣與報告
- `install_busybox_diag_local.sh`：安裝可直接呼叫的 applet 指令
- `benchmark_busybox_diag.sh`：產生效能/大小證據檔


## 11. Benchmark / 效率紀錄

```bash
cd ~/workspace/final_project
scripts/benchmark_busybox_diag.sh ~/workspace/busybox-1.36.1
```

會產生：

- `benchmarks/binary_size.txt`
- `benchmarks/verification_summary.txt`
- `benchmarks/manual_test_summary.txt`

內容包含：

- binary 大小（`ls -lh`、`size`）
- 基本執行紀錄
- 若有 `/usr/bin/time`，則附加 timing
- 若工具不存在，會標示 SKIP

## 12. Release / Freeze 證據檔

重要路徑：

- `release/`
- `release/freeze/`

這些檔案用來保存交付證據，例如：

- build log
- verify log
- demo log
- applet list
- config snapshot
- binary size
- freeze report
- grading bundle manifest

## 13. 移除安裝

```bash
rm -rf /usr/local/busybox-diag
```

若先前有寫入 shell rc，請手動移除這行：

```bash
export PATH=/usr/local/busybox-diag/bin:$PATH
```

## 14. 疑難排解

### `diagfs: command not found`

原因：

- PATH 尚未設定。

解法：

```bash
export PATH=/usr/local/busybox-diag/bin:$PATH
```

### `man` 找不到頁面

man page 屬於可選文件，主要介面是 `--help` 與 `docs/man/*.md`。

若系統已安裝對應 man：

```bash
man diagfs
```

### `ss/netstat` baseline 被跳過

最小化容器可能沒有 `ss` 或 `netstat`。這不代表 `diagnet` 本身失敗。

### `/usr/bin/time` 被跳過

timing 指標是可選項，依賴環境是否有安裝對應工具。

### `Filesystem type: unknown`

常見於容器 overlay 或未映射類型，通常不影響空間/inode 量測結果。

## 15. 各種 PASS 代表什麼

- Build PASS：overlay 與 BusyBox 能成功編譯
- Verify PASS：applet 註冊、CLI/output/JSON、baseline sanity 都通過
- Demo PASS：使用者情境腳本可正常完成
- Applet registration PASS：`diagfs/diagnet/ptop` 出現在 BusyBox applet list
- JSON parse PASS：輸出是可解析 JSON
- Raw pipeline PASS：可用 UNIX pipeline 消費輸出
- Symlink dispatch PASS：透過 applet 名稱可正確 dispatch 到對應 main

## 16. 設計摘要

```text
Kernel interfaces
→ libdiag shared measurement layer
→ applet-specific analysis/policy
→ output formatter
→ BusyBox applet dispatch
```

## 17. 已知限制

- 目前不是 upstream-quality BusyBox patch series
- BusyBox 上游非專題模組的 compile warning 尚未全清
- `diagnet` 依賴 procfs 可見性與容器/主機網路狀態
- `ptop` 不是完整 ncurses top clone
- `diagfs` filesystem type mapping 為輕量策略，部分環境可能顯示 `unknown`
- 最小化容器可能缺 baseline 工具（`ss/netstat/time`）

## 18. 快速指令表

```bash
# 一鍵安裝
cd ~/workspace/final_project
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1

# 若需要手動啟用
export PATH=/usr/local/busybox-diag/bin:$PATH

# 使用
diagfs --output table /
diagnet --output raw
ptop --output table -1 -n 1

# 驗證
cd ~/workspace/busybox-1.36.1
scripts/verify_busybox_diag.sh
```
