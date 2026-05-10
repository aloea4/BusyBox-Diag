#!/bin/bash
# test_diagnet.sh — diagnet acceptance test runner
#
# Usage:
#   cd tools/diagnet && make && ./test_diagnet.sh
#
# Outputs:
#   test_diagnet.log    # full per-case stdout/stderr
#   test_summary.txt    # PASS/FAIL/SKIP summary
#
# Optional dependency: busybox (enables section B cross-check)
#   sudo apt install -y busybox

set -u

DIAGNET=./diagnet
LOG=test_diagnet.log
SUMMARY=test_summary.txt
PASS=0
FAIL=0
SKIP=0

: > "$LOG"

# --- helpers --------------------------------------------------------------

pass() { PASS=$((PASS + 1)); printf 'PASS  %s\n' "$1"; }
fail() { FAIL=$((FAIL + 1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP + 1)); printf 'SKIP  %s\n' "$1"; }

run_case() {
    local name="$1"; shift
    {
        echo "=== $name ==="
        echo "\$ $*"
        eval "$@"
        echo "exit=$?"
        echo
    } >>"$LOG" 2>&1
}

assert_zero() {
    local name="$1"; shift
    run_case "$name" "$@"
    if tail -n 5 "$LOG" | grep -q "^exit=0$"; then
        pass "$name"
    else
        fail "$name (expected exit 0)"
    fi
}

assert_nonzero() {
    local name="$1"; shift
    run_case "$name" "$@"
    if tail -n 5 "$LOG" | grep -q "^exit=0$"; then
        fail "$name (expected non-zero exit)"
    else
        pass "$name"
    fi
}

# --- pre-flight -----------------------------------------------------------

if [[ ! -x "$DIAGNET" ]]; then
    echo "ERROR: $DIAGNET not found or not executable. Run: make"
    exit 2
fi
if ! command -v python3 >/dev/null; then
    echo "ERROR: python3 required for JSON validation"
    exit 2
fi

# --- A. spec acceptance cases ---------------------------------------------

echo "=== Section A: spec acceptance cases ==="

assert_zero    "A1  diagnet"                 "$DIAGNET"
assert_zero    "A2  --proto tcp"             "$DIAGNET --proto tcp"
assert_zero    "A3  --proto udp"             "$DIAGNET --proto udp"
assert_zero    "A4  --stats"                 "$DIAGNET --stats"
assert_zero    "A5  --state ESTABLISHED"     "$DIAGNET --state ESTABLISHED"
assert_zero    "A6  --suspicious"            "$DIAGNET --suspicious"
assert_zero    "A7  --output json | jsonlint" "$DIAGNET --output json | python3 -m json.tool >/dev/null"
assert_zero    "A8  --help"                  "$DIAGNET --help"
assert_zero    "A9  --state INVALID (silent filter)" "$DIAGNET --state INVALID"
assert_nonzero "A10 --whitelist abc"         "$DIAGNET --whitelist abc"
assert_nonzero "A11 --output xml"            "$DIAGNET --output xml"

# --- B. BusyBox netstat cross-check (optional) ----------------------------

echo
echo "=== Section B: BusyBox netstat cross-check ==="

if command -v busybox >/dev/null && busybox netstat -tnl >/dev/null 2>&1; then
    DIAGNET_LISTEN=$("$DIAGNET" --proto tcp --output json 2>/dev/null \
        | python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    ports = sorted({c['local_port'] for c in d['connections'] if c['state'] == 'LISTEN'})
    print(' '.join(str(p) for p in ports))
except Exception as e:
    print('PARSE_ERROR', e, file=sys.stderr)
    sys.exit(1)
")
    BUSY_LISTEN=$(busybox netstat -tnl 2>/dev/null \
        | awk 'NR>2 {n=split($4,a,":"); print a[n]}' \
        | sort -nu | tr '\n' ' ' | sed 's/ $//')
    {
        echo "diagnet LISTEN: [$DIAGNET_LISTEN]"
        echo "busybox LISTEN: [$BUSY_LISTEN]"
    } >>"$LOG"
    if [[ "$DIAGNET_LISTEN" == "$BUSY_LISTEN" ]]; then
        pass "B  netstat cross-check"
    else
        fail "B  netstat cross-check (diagnet=[$DIAGNET_LISTEN] busybox=[$BUSY_LISTEN])"
    fi
else
    skip "B  netstat cross-check (busybox not installed; sudo apt install -y busybox)"
fi

# --- C. JSON schema validation --------------------------------------------

echo
echo "=== Section C: JSON schema ==="

if "$DIAGNET" --output json 2>/dev/null | python3 -c "
import json, sys
d = json.load(sys.stdin)
assert 'connections'      in d, 'missing connections'
assert 'summary'          in d, 'missing summary'
assert 'suspicious_ports' in d, 'missing suspicious_ports'
assert d['summary']['total'] == len(d['connections']), 'total != len(connections)'
print('OK')
" >>"$LOG" 2>&1; then
    pass "C  json schema"
else
    fail "C  json schema"
fi

# --- summary --------------------------------------------------------------

{
    echo "----"
    echo "Total: $((PASS + FAIL + SKIP))   PASS: $PASS   FAIL: $FAIL   SKIP: $SKIP"
    echo "Detail log: $LOG"
} | tee "$SUMMARY"

[[ $FAIL -eq 0 ]] && exit 0 || exit 1
