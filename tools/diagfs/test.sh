#!/bin/bash

# 定義顏色
GREEN='\033[32m'
RED='\033[31m'
NC='\033[0m'

echo "=== diagfs 自動化測試開始 ==="

# 測試 1：正常執行 (預期 Exit Code: 0)
./diagfs / > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} 測試 1: 正常執行"
else
    echo -e "${RED}[FAIL]${NC} 測試 1: 正常執行"
fi

# 測試 2：無效參數 (預期 Exit Code: 2)
./diagfs --output foobar > /dev/null 2>&1
if [ $? -eq 2 ]; then
    echo -e "${GREEN}[PASS]${NC} 測試 2: 無效參數攔截"
else
    echo -e "${RED}[FAIL]${NC} 測試 2: 無效參數攔截"
fi

# 測試 3：無效路徑 (預期 Exit Code: 1)
./diagfs /no_such_dir > /dev/null 2>&1
if [ $? -eq 1 ]; then
    echo -e "${GREEN}[PASS]${NC} 測試 3: 無效路徑攔截"
else
    echo -e "${RED}[FAIL]${NC} 測試 3: 無效路徑攔截"
fi

echo "=== 測試結束 ==="