#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "[1/3] build ptop"
make >/dev/null

echo "[2/3] build + run logic fixture tests"
cc -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L -I. -I../../include \
  tests/test_ptop_logic.c ptop_filter.c ptop_policy.c \
  -o tests/test_ptop_logic.bin
tests/test_ptop_logic.bin

echo "[3/3] runtime output smoke tests"
BATCH_OUT="$(./ptop -1 -b -t 3 -s cpu)"
RAW_OUT="$(./ptop -1 -r -t 2 -s rss)"
JSON_OUT="$(./ptop -1 -j -p 1)"
CPU_FIRST_PID="$(echo "$BATCH_OUT" | awk -F'[ =]' '/^pid=/{print $2; exit}')"
RSS_FIRST_PID="$(echo "$RAW_OUT" | awk -F'[ =]' '/^pid=/{print $2; exit}')"

echo "$BATCH_OUT" | grep -q "^ptop_batch ts="
echo "$BATCH_OUT" | grep -q "mem_total_kb="
echo "$RAW_OUT" | grep -q "^ptop_raw ts="
echo "$JSON_OUT" | grep -q '^{\"timestamp\"'
echo "$JSON_OUT" | grep -q '"processes":\['
test -n "$CPU_FIRST_PID"
test -n "$RSS_FIRST_PID"

echo "all tests passed"

