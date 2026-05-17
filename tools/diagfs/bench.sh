#!/bin/bash

ITERATIONS=100
TARGET_DIR="/"
TEST_FILE="/tmp/test_fiemap.img"

echo "================================================"
echo "  diagfs 效能自動對比測試 (執行 $ITERATIONS 次) "
echo "================================================"

if [ ! -f "$TEST_FILE" ]; then
    echo ">> 找不到測試檔案，正在自動建立 10MB 的 $TEST_FILE ..."
    dd if=/dev/urandom of="$TEST_FILE" bs=1M count=10 > /dev/null 2>&1
    sync
fi

run_benchmark() {
    local cmd="$1"
    local times=()

    # 預熱
    bash -c "$cmd" > /dev/null 2>&1

    for i in $(seq $ITERATIONS); do
        local t_start t_end elapsed_ms
        t_start=$(date +%s%N)
        bash -c "$cmd" > /dev/null 2>&1
        t_end=$(date +%s%N)
        # 轉成毫秒，保留小數
        elapsed_ms=$(awk -v s="$t_start" -v e="$t_end" 'BEGIN { printf "%.3f", (e - s) / 1000000 }')
        times+=("$elapsed_ms")
    done

    local sorted
    sorted=$(printf "%s\n" "${times[@]}" | sort -n)

    local median
    if (( ITERATIONS % 2 == 1 )); then
        local mid=$(( ITERATIONS / 2 + 1 ))
        median=$(echo "$sorted" | sed -n "${mid}p")
    else
        local mid=$(( ITERATIONS / 2 ))
        local a b
        a=$(echo "$sorted" | sed -n "${mid}p")
        b=$(echo "$sorted" | sed -n "$((mid + 1))p")
        median=$(awk -v a="$a" -v b="$b" 'BEGIN { printf "%.3f", (a + b) / 2 }')
    fi

    echo "$median"
}

# ---------------------------------------------------------
echo -e "\n[1] 空間與 inode 掃描對比 (功能對稱化測試)"
echo "------------------------------------------------"

time_orig=$(run_benchmark "df -h $TARGET_DIR && df -i $TARGET_DIR")
time_custom=$(run_benchmark "taskset -c 0 ./diagfs --all --output raw --color never")

echo "原版 (df -h + df -i) 中位數耗時 : $time_orig ms"
echo "你的 diagfs 中位數耗時          : $time_custom ms"

diff_pct=$(awk -v t_orig="$time_orig" -v t_custom="$time_custom" \
    'BEGIN {
        if (t_orig == 0) { print "N/A"; exit }
        printf "%.2f", ((t_custom - t_orig) / t_orig) * 100
    }')
echo "效能落差                        : $diff_pct %"

if [[ "$diff_pct" == "N/A" ]]; then
    echo ">> 結果: [SKIP] 原版耗時為 0ms，解析度仍不足"
elif awk "BEGIN {exit !($diff_pct <= 50.0)}"; then
    echo ">> 結果: [PASS] 恭喜！落差在 50% 以內！"
else
    echo ">> 結果: [FAIL] 警告！落差超過 50%！"
fi

# ---------------------------------------------------------
echo -e "\n[2] 綜合分析對比 (df + filefrag vs diagfs --fiemap)"
echo "------------------------------------------------"

if [ ! -f "$TEST_FILE" ]; then
    echo "找不到測試檔案 $TEST_FILE，跳過此測試"
else
    time_orig_ff=$(run_benchmark "df $TEST_FILE > /dev/null && filefrag -v $TEST_FILE")
    time_custom_ff=$(run_benchmark "./diagfs --fiemap $TEST_FILE --output raw --color never")

    echo "原版工具組合中位數耗時 : $time_orig_ff ms"
    echo "你的 diagfs 中位數耗時 : $time_custom_ff ms"

    diff_pct_ff=$(awk -v t_orig="$time_orig_ff" -v t_custom="$time_custom_ff" \
        'BEGIN {
            if (t_orig == 0) { print "N/A"; exit }
            printf "%.2f", ((t_custom - t_orig) / t_orig) * 100
        }')
    echo "效能落差               : $diff_pct_ff %"

    if [[ "$diff_pct_ff" == "N/A" ]]; then
        echo ">> 結果: [SKIP] 原版耗時為 0ms，解析度仍不足"
    elif awk "BEGIN {exit !($diff_pct_ff <= 50.0)}"; then
        echo ">> 結果: [PASS] 恭喜！落差在 50% 以內！"
    else
        echo ">> 結果: [FAIL] 警告！落差超過 50%！"
    fi
fi