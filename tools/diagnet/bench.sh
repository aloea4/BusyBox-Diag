#!/bin/bash
# bench.sh — diagnet performance benchmark vs ss
#
# Usage:
#   cd tools/diagnet && make && ./bench.sh [N]
#
# N: iterations per case (default 200, must be >= 5)
#
# Outputs:
#   bench.log    # full per-iteration timings
#   stdout       # human-readable summary table
#
# Spec: spec requires diagnet within 50% of native tool — i.e. diagnet/ss
# ratio must be <= 1.50. Cases exceeding this are flagged WARNING.

set -u

N="${1:-200}"
DIAGNET=./diagnet
LOG=bench.log
: > "$LOG"

if [[ ! -x "$DIAGNET" ]]; then
    echo "ERROR: $DIAGNET not built. Run: make" >&2
    exit 2
fi
if ! command -v ss >/dev/null; then
    echo "ERROR: ss(8) not found (apt install iproute2)" >&2
    exit 2
fi
if (( N < 5 )); then
    echo "ERROR: N must be >= 5 (got $N)" >&2
    exit 2
fi

# Time a single command's wall-clock in seconds (float). Uses bash 5+
# EPOCHREALTIME if available, otherwise falls back to date +%s.%N.
now_s() {
    if [[ -n "${EPOCHREALTIME:-}" ]]; then
        echo "$EPOCHREALTIME"
    else
        date +%s.%N
    fi
}

run_once() {
    local cmd="$1"
    local t0 t1
    t0=$(now_s)
    eval "$cmd" >/dev/null 2>&1
    t1=$(now_s)
    awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.6f\n", b - a }'
}

WARMUP=20

# Optional CPU pinning to reduce migration noise. Set DIAGNET_PIN=1 to enable.
if [[ "${DIAGNET_PIN:-0}" == "1" ]] && command -v taskset >/dev/null; then
    echo "(pinning measurements to CPU 0 via taskset)"
    PIN="taskset -c 0"
else
    PIN=""
fi

# Run N+WARMUP iterations (WARMUP discarded), then compute stats.
bench_case() {
    local label="$1" cmd="$2"
    local i samples=()

    # warmup
    for ((i = 0; i < WARMUP; i++)); do
        run_once "$PIN $cmd" >/dev/null
    done

    for ((i = 0; i < N; i++)); do
        samples+=("$(run_once "$PIN $cmd")")
    done

    {
        echo "=== $label ==="
        echo "cmd: $cmd"
        printf '%s\n' "${samples[@]}"
        echo
    } >> "$LOG"

    printf '%s\n' "${samples[@]}" | awk -v label="$label" '
        { x[NR]=$1; sum+=$1 }
        END {
            n = NR
            mean = sum / n
            for (i = 1; i <= n; i++) {
                d = x[i] - mean
                ssq += d * d
            }
            stddev = (n > 1) ? sqrt(ssq / (n - 1)) : 0
            # crude median via insertion sort
            for (i = 1; i <= n; i++) y[i] = x[i]
            for (i = 2; i <= n; i++) {
                k = y[i]; j = i - 1
                while (j > 0 && y[j] > k) { y[j+1] = y[j]; j-- }
                y[j+1] = k
            }
            median = (n % 2) ? y[(n+1)/2] : (y[n/2] + y[n/2+1]) / 2
            printf "%s\tmean=%.6fs\tmedian=%.6fs\tstddev=%.6fs\n",
                   label, mean, median, stddev
        }'
}

# Capture mean / median for a label out of bench_case output.
parse_mean() {
    awk -v lbl="$1" -F'\t' '$1 == lbl { sub(/mean=/, "", $2); sub(/s$/, "", $2); print $2 }'
}
parse_median() {
    awk -v lbl="$1" -F'\t' '$1 == lbl { sub(/median=/, "", $3); sub(/s$/, "", $3); print $3 }'
}

echo "diagnet benchmark — N=$N iterations per case (plus $WARMUP warmup)"
echo "kernel:  $(uname -r)"
echo "host:    $(uname -n)"
echo "date:    $(date -u +%FT%TZ)"
echo

# Use a temp file so we can pair diagnet/ss results and compute ratios.
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

{
    # NOTE: ss without -a skips LISTEN sockets ("open non-listening sockets that
    # have established connection" per ss(8)), while diagnet shows ALL TCP/UDP
    # by default. Adding -a aligns ss coverage with diagnet for an apples-to-
    # apples comparison.
    bench_case "diagnet-raw"   "$DIAGNET --proto tcp --output raw"
    bench_case "ss-raw"        "ss -tna"
    bench_case "diagnet-json"  "$DIAGNET --proto tcp --output json"
    bench_case "ss-json"       "ss -tna -O"
    bench_case "diagnet-stats" "$DIAGNET --stats"
    bench_case "ss-stats"      "ss -tnua"
} | tee "$TMP"

echo
echo "Comparison (diagnet / ss; verdict uses median; target <= 1.50):"
printf '%-22s %12s %12s %10s %10s %s\n' \
       "case" "diag med(s)" "ss med(s)" "ratio-med" "ratio-mean" "verdict"

compare() {
    local case_label="$1" diag_label="$2" ss_label="$3"
    local dm sm dmn smn r_med r_mean verdict
    dm=$(parse_median "$diag_label" < "$TMP")
    sm=$(parse_median "$ss_label"   < "$TMP")
    dmn=$(parse_mean "$diag_label" < "$TMP")
    smn=$(parse_mean "$ss_label"   < "$TMP")
    if [[ -z "$dm" || -z "$sm" ]]; then
        printf '%-22s %12s %12s %10s %10s %s\n' \
               "$case_label" "?" "?" "?" "?" "SKIP"
        return
    fi
    r_med=$(awk -v a="$dm" -v b="$sm" 'BEGIN { if (b == 0) print "inf"; else printf "%.2f", a / b }')
    r_mean=$(awk -v a="$dmn" -v b="$smn" 'BEGIN { if (b == 0) print "inf"; else printf "%.2f", a / b }')
    verdict=$(awk -v r="$r_med" 'BEGIN { if (r == "inf") print "?"; else if (r <= 1.50) print "OK"; else print "WARNING(>50%)" }')
    printf '%-22s %12.6f %12.6f %10s %10s %s\n' \
           "$case_label" "$dm" "$sm" "$r_med" "$r_mean" "$verdict"
}

compare "tcp listing (raw)"      "diagnet-raw"   "ss-raw"
compare "tcp listing (json)"     "diagnet-json"  "ss-json"
compare "state distribution"     "diagnet-stats" "ss-stats"

echo
echo "Full per-iteration timings: $LOG"
