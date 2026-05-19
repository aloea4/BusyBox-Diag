#!/usr/bin/env bash
set -euo pipefail

require_busybox_root() {
    local missing=0
    for p in Makefile Config.in applets include miscutils; do
        if [ ! -e "$p" ]; then
            echo "ERROR: missing required BusyBox root path: $p" >&2
            missing=1
        fi
    done
    if [ "$missing" -ne 0 ]; then
        echo "ERROR: this script must run from BusyBox source root" >&2
        exit 1
    fi
}

enable_config() {
    local key="$1"
    if [ -x scripts/config ]; then
        scripts/config -e "$key"
    else
        if grep -q "^# CONFIG_${key} is not set" .config; then
            sed -i "s/^# CONFIG_${key} is not set/CONFIG_${key}=y/" .config
        elif grep -q "^CONFIG_${key}=" .config; then
            sed -i "s/^CONFIG_${key}=.*/CONFIG_${key}=y/" .config
        else
            echo "CONFIG_${key}=y" >> .config
        fi
    fi
}

require_busybox_root

LOG_FILE="${PWD}/build_busybox_diag.log"
exec > >(tee "$LOG_FILE") 2>&1

echo "[INFO] BusyBox-Diag reproducible build start"
echo "[INFO] Working directory: $PWD"
echo "[INFO] Log file: $LOG_FILE"

make defconfig

enable_config DIAGFS
enable_config DIAGNET
enable_config PTOP
enable_config LIBDIAG

# avoid SIGPIPE false-negative under `set -o pipefail`
set +o pipefail
yes "" | make oldconfig
set -o pipefail

if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
else
    JOBS=2
fi
make -j"$JOBS"

if [ ! -x ./busybox ]; then
    echo "ERROR: ./busybox not found or not executable" >&2
    exit 1
fi

./busybox --list | grep -x diagfs
./busybox --list | grep -x diagnet
./busybox --list | grep -x ptop

./busybox diagfs --help >/dev/null
./busybox diagnet --help >/dev/null
./busybox ptop --help >/dev/null

./busybox diagfs --output raw / >/dev/null
./busybox diagnet --output raw >/dev/null
./busybox ptop --output table -1 -n 1 >/dev/null

if command -v python3 >/dev/null 2>&1; then
    ./busybox diagfs --output json / | python3 -m json.tool >/dev/null
    ./busybox diagnet --output json | python3 -m json.tool >/dev/null
    ./busybox ptop --output json -1 -n 1 | python3 -m json.tool >/dev/null
else
    echo "[INFO] python3 not found, skip JSON parse validation"
fi

echo "BusyBox-Diag build and smoke tests PASSED"
