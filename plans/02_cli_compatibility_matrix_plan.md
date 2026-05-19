# 計畫 2：Toybox/GNU CLI 相容矩陣（草案）

## 1. 目標

建立可驗收的 CLI 相容基線，讓每個工具都能回答：

- 與哪個參考工具對齊（Toybox/GNU/自訂）
- 哪些選項是 fully-compatible / alias-compatible / intentionally-different
- 差異是否有文件與測試覆蓋

## 2. 相容性定義

- **L1 Full**：語法、語意、輸出欄位與 exit code 皆相容
- **L2 Alias**：語意相容，但命名透過 alias 提供
- **L3 Partial**：主要用途相容，但有受限差異
- **L4 Custom**：明確為專題自訂，不宣稱相容

## 3. 工具對照目標

- `ptop` 對照：`top` / `toybox top`（以「輕量 snapshot 監控」語意對齊，不宣稱 htop 完整互動）
- `diagfs` 對照：`df`, `stat`, `filefrag`（組合語意對齊）
- `diagnet` 對照：`ss`, `netstat`（連線 metadata listing + filter）

## 4. 產出物

1. `docs/compatibility_matrix.md`
2. `docs/compatibility_decisions.md`
3. 自動化 smoke 測試補充：
   - unknown option → usage error
   - output mode contract
   - header/quiet 行為

## 5. 矩陣欄位建議

- Option
- Reference Tool Syntax
- BusyBox-Diag Syntax
- Compatibility Level（L1~L4）
- Reason / Notes
- Test Case ID

## 6. 初始缺口（預估）

- `ptop` 尚未有 `--no-header` / `--quiet`
- `diagnet` exit code 3 為 reserved（文件須持續一致）
- 各工具 table/raw 欄位命名仍有局部差異

## 7. 執行步驟

1. 盤點三工具 help/man/options
2. 建立第一版矩陣
3. 定義差異是「要補齊」或「保留設計差異」
4. 補測試、補文件
5. 凍結相容契約（版本化）

## 8. 驗收標準

- 每個公開選項都有矩陣條目
- 每個非相容差異都有明確理由與測試
- README/man 與矩陣內容一致

