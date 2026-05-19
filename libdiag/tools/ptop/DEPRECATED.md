# DEPRECATED: libdiag/tools/ptop

此目錄為歷史副本，**不再作為主維護來源**。

## 目前唯一主來源

- `tools/ptop`

## 目前唯一建置/測試入口

```bash
make -C tools/ptop standalone
bash tools/ptop/tests/run_tests.sh
```

## 維護規則

- 新功能、修 bug、測試修正請只改 `tools/ptop`。
- 不要在本目錄做新變更，避免雙份漂移。

