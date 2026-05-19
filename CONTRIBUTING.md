# Contributing to BusyBox-Diag

感謝你對 BusyBox-Diag 的貢獻。  
本文件提供最小但完整的貢獻流程，適用於課程專題與後續維護。

## 1. Scope

BusyBox-Diag 是 BusyBox overlay 專案，重點是：

- `diagfs`：Filesystem metadata monitoring tool
- `diagnet`：Passive kernel socket state observer
- `ptop`：Snapshot-based lightweight process monitor
- `libdiag`：shared measurement layer

請優先維持：

- BusyBox applet integration
- CLI 相容性與既有行為穩定
- 可重現 build / verify / benchmark 流程

## 2. Repository Layout

- `busybox_overlay/`：要套用到 BusyBox source tree 的核心程式碼
- `scripts/`：自動化腳本（apply/build/verify/bootstrap/install/benchmark）
- `docs/`：架構、手冊、man 文件、相容性文件
- `benchmarks/`：基準測試資料與報告
- `release/`：交付與 release 文件

## 3. Development Prerequisites

建議環境：

- Linux / container
- `gcc`, `make`, `python3`, `wget`, `tar`
- BusyBox 1.36.1 source tree

## 4. Standard Workflow

1. 準備 BusyBox source tree
2. 在本 repo 執行 one-shot bootstrap：

```bash
scripts/bootstrap_busybox_diag.sh /path/to/busybox-1.36.1 --no-activate
```

3. 若有程式變更，重跑驗證：

```bash
cd /path/to/busybox-1.36.1
scripts/build_busybox_diag.sh
scripts/verify_busybox_diag.sh
```

4. 若有效能相關變更，重跑 benchmark：

```bash
cd /path/to/final_project
scripts/benchmark_all_required.sh --busybox-root /path/to/busybox-1.36.1
```

## 5. Coding Rules (Project-specific)

- 不要把 tool-specific policy 放進 `libdiag`
- 不要破壞既有 CLI 與輸出 schema
- 不做非必要的大型重構
- 優先維持 BusyBox 整合可用性與腳本可重現性

## 6. Commit & PR Guidelines

### Commit message 建議

使用清楚、可追溯的訊息，例如：

- `docs: update USER_MANUAL benchmark section`
- `scripts: add benchmark_all_required runner`
- `diagfs: align table wording with docs`

### Pull Request 內容至少包含

1. 變更摘要（改了哪些檔案/模組）
2. 原因（為什麼需要這個改動）
3. 驗證結果（貼必要指令與 PASS/FAIL）
4. 是否影響：
   - CLI 行為
   - 輸出格式
   - benchmark 結果
   - 文件

## 7. Required Checks Before Merge

若變更涉及功能或腳本，至少確認：

```bash
# in final_project
scripts/bootstrap_busybox_diag.sh /path/to/busybox-1.36.1 --no-activate

# in busybox source root
scripts/build_busybox_diag.sh
scripts/verify_busybox_diag.sh
```

若變更涉及效能或交付報告，請再執行：

```bash
cd /path/to/final_project
scripts/benchmark_all_required.sh --busybox-root /path/to/busybox-1.36.1 --skip-bootstrap
```

## 8. Documentation Policy

當你修改以下任一項時，請同步更新文件：

- CLI 選項或輸出：更新 `docs/USER_MANUAL.md` 與 `docs/man/*`
- 相容性決策：更新 `docs/compatibility_matrix.md` / `docs/compatibility_decisions.md`
- benchmark 流程：更新 `docs/USER_MANUAL.md` 的 benchmark 章節

## 9. Known Limitations

目前已知限制（提交時可直接引用）：

- 專案目標是課程交付可重現流程，不是 upstream-quality BusyBox patch series
- 部分環境可能缺 `ss/netstat` 或 `/usr/bin/time`，benchmark 會出現 SKIP
- `ptop` 非完整 ncurses top clone

## 10. Code of Conduct

請保持尊重、透明與可追溯的協作方式：

- 尊重既有設計與提交者
- 任何重大變更先說明取捨
- 讓下一位維護者可直接重現你的結果
