#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage:
  scripts/benchmark_all_required.sh [--busybox-root PATH] [--warmup N] [--samples N] [--skip-bootstrap]

Description:
  Run all required BusyBox-Diag benchmark checks in one command:
    1) optional bootstrap (overlay/build/verify/install)
    2) delivery benchmark artifacts (binary_size/verification_summary/manual_test_summary)
    3) raw timing benchmark CSV + summary report

Options:
  --busybox-root PATH   BusyBox source root (default: /root/workspace/busybox-1.36.1)
  --warmup N            Warmup rounds per case (default: 20)
  --samples N           Measured rounds per case (default: 200)
  --skip-bootstrap      Skip bootstrap phase
USAGE
}

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUSYBOX_ROOT="/root/workspace/busybox-1.36.1"
WARMUP=20
SAMPLES=200
SKIP_BOOTSTRAP=0

while [ $# -gt 0 ]; do
    case "$1" in
        --busybox-root)
            BUSYBOX_ROOT="${2:-}"
            shift 2
            ;;
        --warmup)
            WARMUP="${2:-}"
            shift 2
            ;;
        --samples)
            SAMPLES="${2:-}"
            shift 2
            ;;
        --skip-bootstrap)
            SKIP_BOOTSTRAP=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [ ! -f "${BUSYBOX_ROOT}/Makefile" ] || [ ! -d "${BUSYBOX_ROOT}/applets" ] || [ ! -d "${BUSYBOX_ROOT}/miscutils" ]; then
    echo "ERROR: invalid BusyBox source root: ${BUSYBOX_ROOT}" >&2
    exit 1
fi

if ! [[ "${WARMUP}" =~ ^[0-9]+$ ]] || ! [[ "${SAMPLES}" =~ ^[0-9]+$ ]]; then
    echo "ERROR: --warmup and --samples must be non-negative integers" >&2
    exit 1
fi

cd "${ROOT_DIR}"
mkdir -p benchmarks/raw_results benchmarks/research_notes

if [ "${SKIP_BOOTSTRAP}" -eq 0 ]; then
    echo "[1/4] Bootstrap BusyBox-Diag"
    scripts/bootstrap_busybox_diag.sh "${BUSYBOX_ROOT}" --no-activate
else
    echo "[1/4] Bootstrap skipped by option"
fi

echo "[2/4] Generate delivery benchmark artifacts"
scripts/benchmark_busybox_diag.sh "${BUSYBOX_ROOT}"

BUSYBOX_BIN="${BUSYBOX_ROOT}/busybox"
if [ ! -x "${BUSYBOX_BIN}" ]; then
    echo "ERROR: busybox binary not found: ${BUSYBOX_BIN}" >&2
    exit 1
fi

echo "[3/4] Run raw timing benchmark (WARMUP=${WARMUP}, SAMPLES=${SAMPLES})"
TS="$(date +%Y%m%d_%H%M%S)"
RAW_CSV="benchmarks/raw_results/benchmark_${TS}.csv"
SUMMARY_MD="benchmarks/research_notes/final_report_${TS}.md"

printf "case_id,tool,variant,time_sec\n" > "${RAW_CSV}"

now_ns() { date +%s%N; }

time_cmd() {
    local cmd="$1"
    local t1 t2
    t1="$(now_ns)"
    bash -lc "$cmd" >/dev/null 2>&1
    t2="$(now_ns)"
    awk -v a="${t1}" -v b="${t2}" 'BEGIN { printf "%.9f", (b-a)/1000000000.0 }'
}

run_case() {
    local case_id="$1"
    local tool="$2"
    local custom_cmd="$3"
    local ref_cmd="$4"

    local i t_custom t_ref
    for i in $(seq 1 "${WARMUP}"); do
        bash -lc "${custom_cmd}" >/dev/null 2>&1 || true
        bash -lc "${ref_cmd}" >/dev/null 2>&1 || true
    done

    for i in $(seq 1 "${SAMPLES}"); do
        t_custom="$(time_cmd "${custom_cmd}")"
        printf "%s,%s,custom,%s\n" "${case_id}" "${tool}" "${t_custom}" >> "${RAW_CSV}"

        t_ref="$(time_cmd "${ref_cmd}")"
        printf "%s,%s,reference,%s\n" "${case_id}" "${tool}" "${t_ref}" >> "${RAW_CSV}"
    done
}

run_case "M3-PTOP-01" "ptop" "${BUSYBOX_BIN} ptop --output table -1 -n 1" "top -b -d 1 -n 2"
run_case "M3-DIAGFS-01" "diagfs" "${BUSYBOX_BIN} diagfs --output raw /" "df -k /"

if command -v ss >/dev/null 2>&1; then
    run_case "M3-DIAGNET-01" "diagnet" "${BUSYBOX_BIN} diagnet --output raw" "ss -tna"
elif command -v netstat >/dev/null 2>&1; then
    run_case "M3-DIAGNET-01" "diagnet" "${BUSYBOX_BIN} diagnet --output raw" "netstat -tna"
else
    echo "[WARN] skip M3-DIAGNET-01 raw benchmark: ss/netstat not found"
fi

awk -F, '
BEGIN {
  print "# Benchmark Final Report\n";
  print "- generated: " strftime("%Y-%m-%d %H:%M:%S");
  print "- raw csv: " RAW "\n";

  print "| Case | Tool | custom_median | reference_median | ratio | 50%-gap target |";
  print "|---|---:|---:|---:|---:|---|";
}
NR>1 {
  key=$1"|"$2"|"$3;
  n[key]++;
  a[key,n[key]]=$4+0;
}
END {
  split("M3-PTOP-01|ptop M3-DIAGFS-01|diagfs M3-DIAGNET-01|diagnet", cases, " ");
  for (i in cases) {
    split(cases[i], p, "|");
    cid=p[1]; tool=p[2];
    kc=cid"|"tool"|custom";
    kr=cid"|"tool"|reference";

    if (n[kc] == 0 || n[kr] == 0) {
      printf "| %s | %s | - | - | - | SKIP |\n", cid, tool;
      continue;
    }

    mc=median(kc);
    mr=median(kr);
    ratio=(mr>0)? mc/mr : 999;
    verdict=(ratio<=1.50)?"PASS":"FAIL";
    printf "| %s | %s | %.6f | %.6f | %.3f | %s |\n", cid, tool, mc, mr, ratio, verdict;
  }
}
function median(k,   i,j,tmp,cnt,vals,mid) {
  cnt=n[k];
  if (cnt==0) return 0;
  for (i=1;i<=cnt;i++) vals[i]=a[k,i];
  for (i=1;i<=cnt;i++) for (j=i+1;j<=cnt;j++) if (vals[i]>vals[j]) { tmp=vals[i]; vals[i]=vals[j]; vals[j]=tmp }
  if (cnt%2==1) { mid=(cnt+1)/2; return vals[mid]; }
  mid=cnt/2; return (vals[mid]+vals[mid+1])/2;
}
' RAW="${RAW_CSV}" "${RAW_CSV}" > "${SUMMARY_MD}"

LATEST_LINK="benchmarks/research_notes/final_report_latest.md"
cp "${SUMMARY_MD}" "${LATEST_LINK}"

echo "[4/4] Done"
echo "Artifacts:"
echo "  ${RAW_CSV}"
echo "  ${SUMMARY_MD}"
echo "  ${LATEST_LINK}"
echo "  benchmarks/binary_size.txt"
echo "  benchmarks/verification_summary.txt"
echo "  benchmarks/manual_test_summary.txt"
