#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage:
  scripts/bootstrap_busybox_diag.sh /path/to/busybox-source [--no-activate]

Description:
  One-shot BusyBox-Diag bootstrap:
    1) apply overlay
    2) sync helper scripts into BusyBox tree
    3) build
    4) verify
    5) local install

Options:
  --no-activate   Install without appending PATH to /root/.bashrc
USAGE
}

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    usage
    exit 1
fi

TARGET="$1"
ACTIVATE="yes"
if [ "${2:-}" = "--no-activate" ]; then
    ACTIVATE="no"
elif [ $# -eq 2 ]; then
    echo "ERROR: unknown option: $2" >&2
    usage
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ ! -f "${TARGET}/Makefile" ] || [ ! -d "${TARGET}/applets" ] || [ ! -d "${TARGET}/miscutils" ]; then
    echo "ERROR: target is not a BusyBox source tree: ${TARGET}" >&2
    exit 1
fi

echo "[1/5] Applying overlay to ${TARGET}"
"${SCRIPT_DIR}/apply_overlay.sh" "${TARGET}"

echo "[2/5] Syncing helper scripts into ${TARGET}/scripts"
mkdir -p "${TARGET}/scripts"
cp "${SCRIPT_DIR}/build_busybox_diag.sh" "${TARGET}/scripts/"
cp "${SCRIPT_DIR}/verify_busybox_diag.sh" "${TARGET}/scripts/"
cp "${SCRIPT_DIR}/install_busybox_diag_local.sh" "${TARGET}/scripts/"
chmod +x "${TARGET}/scripts/build_busybox_diag.sh" \
         "${TARGET}/scripts/verify_busybox_diag.sh" \
         "${TARGET}/scripts/install_busybox_diag_local.sh"

echo "[3/5] Building BusyBox-Diag"
(
    cd "${TARGET}"
    ./scripts/build_busybox_diag.sh
)

echo "[4/5] Verifying BusyBox-Diag"
(
    cd "${TARGET}"
    ./scripts/verify_busybox_diag.sh
)

echo "[5/5] Installing BusyBox-Diag applets"
(
    cd "${TARGET}"
    if [ "${ACTIVATE}" = "yes" ]; then
        ./scripts/install_busybox_diag_local.sh --activate
    else
        ./scripts/install_busybox_diag_local.sh
    fi
)

echo
echo "Bootstrap completed successfully."
echo "You can run:"
echo "  diagfs --help"
echo "  diagnet --help"
echo "  ptop --help"
if [ "${ACTIVATE}" = "yes" ]; then
    echo "If current shell does not see PATH update yet, run: source /root/.bashrc"
fi
