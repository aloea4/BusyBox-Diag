# ptop 單一來源遷移說明

## 結論

`ptop` 的唯一主維護來源為：

- `tools/ptop`

`libdiag/tools/ptop` 僅保留為歷史副本，已標記 deprecated。

## 為什麼要收斂

- 避免雙份程式碼漂移
- 避免測試、文件、實作不同步
- 降低接手與自動化代理的判斷成本

## 新的標準操作

```bash
# build
make -C tools/ptop standalone

# tests
bash tools/ptop/tests/run_tests.sh
```

## 不應再使用的入口

- `make -C libdiag/tools/ptop ...`
- `bash libdiag/tools/ptop/tests/run_tests.sh`

## 驗證

- `tools/ptop` build/test 必須可獨立成功
- 活躍 build/test 腳本不再依賴 `libdiag/tools/ptop`

