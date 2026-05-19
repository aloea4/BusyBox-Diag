# 計畫 4：開源發布與文件封裝（草案）

## 1. 目標

滿足專題的開源交付要求：

- GitHub 獨立專案可發布
- 文件完整：`README + CONTRIBUTING + man pages`
- 對外可重現 build/test/benchmark

## 2. 交付物清單

1. `README.md`（已強化，後續只做一致性維護）
2. `CONTRIBUTING.md`（待新增）
3. `CODE_OF_CONDUCT.md`（建議）
4. `LICENSE`（依課程/團隊決策）
5. `docs/architecture.md`（可從現有 README 拆分）
6. man pages：
   - `tools/diagfs/diagfs.1`
   - `tools/diagnet/diagnet.1`
   - `tools/ptop/ptop.1`（若目前缺，需補）

## 3. CONTRIBUTING 建議章節

- 開發環境需求
- build/test 指令
- commit 訊息規範
- PR checklist
- CLI/exit code/documentation 同步要求
- benchmark 變更提交流程

## 4. GitHub 發布流程

1. 清理 branch（無臨時產物）
2. 建立版本標記（`v0.x.y`）
3. release note：
   - 功能
   - 相容性
   - 已知限制
4. 上傳 benchmark 報告與主要文件連結

## 5. CI 建議

最小 CI：

- build：`libdiag + 3 tools`
- test：`diagfs/diagnet/ptop` 腳本
- docs lint（可選）

## 6. 風險與緩解

- 風險：文件與行為漂移
  - 緩解：PR 模板強制同步 check
- 風險：man page 落後
  - 緩解：每次 CLI 變更必改 `.1`

## 7. 驗收標準

- 第三方 clone 後可依 README 成功 build/test
- `CONTRIBUTING.md` 完整可操作
- man pages 與 `--help` 行為一致
- release artifact 能支持專題審查

