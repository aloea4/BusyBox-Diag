#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage:"
    echo "apply_overlay.sh /path/to/busybox-source"
    exit 1
fi

TARGET="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OVERLAY_DIR="${ROOT_DIR}/busybox_overlay"

if [ ! -f "${TARGET}/Makefile" ] || [ ! -d "${TARGET}/applets" ] || [ ! -d "${TARGET}/miscutils" ]; then
    echo "ERROR: target is not a BusyBox source tree: ${TARGET}" >&2
    exit 1
fi

mkdir -p "${TARGET}/miscutils/libdiag" "${TARGET}/include/libdiag"

cp "${OVERLAY_DIR}"/miscutils/diagfs*.c "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/diagfs*.h "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/diagnet*.c "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/diagnet*.h "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/ptop*.c "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/ptop*.h "${TARGET}/miscutils/"
cp "${OVERLAY_DIR}"/miscutils/libdiag/*.c "${TARGET}/miscutils/libdiag/"
cp "${OVERLAY_DIR}"/include/libdiag/*.h "${TARGET}/include/libdiag/"

echo "Overlay applied successfully."
echo "Now run:"
echo "scripts/build_busybox_diag.sh"
