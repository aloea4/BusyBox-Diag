# BusyBox-Diag 系統架構

## 1. Project Overview

BusyBox-Diag 是一套 **lightweight BusyBox-integrated diagnostic toolkit**，目標是在單一 BusyBox binary 中提供 process/filesystem/network 三類診斷能力，並維持 BusyBox 風格的部署與執行模型。

## 2. Tool Overview

| Tool | Role |
| --- | --- |
| diagfs | filesystem metadata monitoring |
| diagnet | passive kernel socket state observer |
| ptop | snapshot-based lightweight process monitor |

## 3. libdiag Role

`libdiag` 是三個工具共用的 **shared measurement layer**，負責提供對核心資訊來源的讀取與解析抽象，核心定位是：

- procfs/statfs parsing abstraction
- 統一 measurement 資料結構
- 工具共用的底層資料採集

## 4. Layered Architecture

```text
Kernel
 ↓
/proc / statfs
 ↓
libdiag
 ↓
tool collect layer
 ↓
analysis/policy
 ↓
output layer
 ↓
BusyBox applet dispatch
```

## 5. BusyBox Integration

目前整合採 BusyBox applet 模式，重點如下：

- applet registration：三個工具註冊為 `diagfs` / `diagnet` / `ptop`
- Kconfig：以 `CONFIG_DIAGFS` / `CONFIG_DIAGNET` / `CONFIG_PTOP` / `CONFIG_LIBDIAG` 控制
- Kbuild：將工具物件檔與 `libdiag` 量測層編入同一 build graph
- single binary integration：最終都進入 `./busybox` 單一執行檔
- argv[0] dispatch：透過 BusyBox dispatch 機制導向對應 `*_main()`

## 6. Design Goals

- lightweight
- shared code
- BusyBox compatibility
- script-friendly output
- low dependency
