# 執行清單：M4 開源發布與文件封裝

## 任務 1：新增 CONTRIBUTING
- 優先級：`P0`
- 預估工時：`1.0h`
- 狀態：`DONE`
- 執行證據：
  - 已新增 `CONTRIBUTING.md`（含 build/test、架構邊界、CLI/docs 同步、benchmark 規範、PR checklist）。
- 驗收指令：
```bash
test -f CONTRIBUTING.md
sed -n '1,260p' CONTRIBUTING.md
```

## 任務 2：README/man 最終校稿
- 優先級：`P1`
- 預估工時：`1.5h`
- 狀態：`DONE`
- 執行證據：
  - README 已補相容矩陣摘要與 Test Case ID 導覽。
  - `diagnet` help/man exit code 文案一致化（`3=reserved (unused)`）。
  - 已完成 cross-check：`diagfs/diagnet/ptop --help` 均可執行，README/man/docs 關鍵字一致。
- 驗收指令：
```bash
./tools/diagfs/diagfs -h >/dev/null
./tools/diagnet/diagnet --help >/dev/null
./tools/ptop/ptop --help >/dev/null
rg -n "Exit|Usage|output|compatibility" README.md tools/diagfs/diagfs.1 tools/diagnet/diagnet.1 docs
```

## 任務 3：release checklist + 版本流程
- 優先級：`P1`
- 預估工時：`1.0h`
- 狀態：`DONE`
- 執行證據：
  - 已新增 `docs/release_checklist.md`。
  - 含版本標記流程、Release Gate、Release Note 模板。
- 驗收指令：
```bash
test -f docs/release_checklist.md
rg -n "Git Tag|Release Gate|Release Note 模板|WARMUP=20|ratio <= 1.50" docs/release_checklist.md
```

## 任務 4：CI 最小流程
- 優先級：`P1`
- 預估工時：`2.0h`
- 狀態：`DONE`
- 執行證據：
  - 已新增 `.github/workflows/ci.yml`。
  - 包含 build：`libdiag + diagfs + diagnet + ptop`。
  - 包含 tests：`tools/diagfs/test.sh`、`tools/diagnet/test_diagnet.sh`、`tools/ptop/tests/run_tests.sh`。
- 驗收指令：
```bash
test -f .github/workflows/ci.yml
sed -n '1,220p' .github/workflows/ci.yml
```

## 任務 5：治理檔補齊（可選）
- 優先級：`P2`
- 預估工時：`1.0h`
- 狀態：`TODO`
- 建議內容：
  - `LICENSE`
  - `CODE_OF_CONDUCT.md`
- 驗收指令：
```bash
ls LICENSE CODE_OF_CONDUCT.md
```

