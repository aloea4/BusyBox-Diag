# BusyBox-Diag

BusyBox-Diag 是一套整合進 BusyBox 的輕量系統診斷子系統。
它會把三個 applet 編進同一個 BusyBox binary：

- `diagfs`：檔案系統中繼資料監控工具
- `diagnet`：被動式核心 socket 狀態觀察器
- `ptop`：snapshot-based 輕量行程監控工具

整體使用共用的 `libdiag` measurement layer，並以 overlay 方式套用到 BusyBox source tree。

## 快速開始

### 1) 準備 BusyBox 原始碼

```bash
cd ~/workspace
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2
```

### 2) 一鍵 bootstrap（建議）

```bash
cd ~/workspace/final_project
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1
```

若你不希望自動修改 shell rc（例如 `~/.bashrc`）：

```bash
scripts/bootstrap_busybox_diag.sh ~/workspace/busybox-1.36.1 --no-activate
```

### 3) 直接使用指令

```bash
export PATH=/usr/local/busybox-diag/bin:$PATH

diagfs --output table /
diagnet --output raw
ptop --output table -1 -n 1
```

## 驗證

```bash
cd ~/workspace/busybox-1.36.1
scripts/verify_busybox_diag.sh
```

## 完整使用手冊

詳細逐步教學請看：

- `docs/USER_MANUAL.md`

## 主要目錄入口

- Overlay 原始碼：`busybox_overlay/`
- 自動化腳本：`scripts/`
- 文件：`docs/`
- Release 產物：`release/`
- Benchmark 產物：`benchmarks/`
