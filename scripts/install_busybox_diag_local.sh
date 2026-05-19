#!/usr/bin/env bash
set -euo pipefail

PREFIX="/usr/local/busybox-diag"
BINDIR="${PREFIX}/bin"
BUSYBOX_SRC="./busybox"
BUSYBOX_DST="${BINDIR}/busybox"

if [ ! -f Makefile ] || [ ! -d applets ] || [ ! -d miscutils ]; then
    echo "ERROR: run this script from BusyBox source root" >&2
    exit 1
fi

if [ ! -x "$BUSYBOX_SRC" ]; then
    echo "ERROR: ./busybox not found or not executable. Run scripts/build_busybox_diag.sh first." >&2
    exit 1
fi

mkdir -p "$BINDIR"

cp "$BUSYBOX_SRC" "$BUSYBOX_DST"
chmod +x "$BUSYBOX_DST"

ln -sf busybox "${BINDIR}/diagfs"
ln -sf busybox "${BINDIR}/diagnet"
ln -sf busybox "${BINDIR}/ptop"

"${BINDIR}/diagfs" --help >/dev/null
"${BINDIR}/diagnet" --help >/dev/null
"${BINDIR}/ptop" --help >/dev/null

echo "BusyBox-Diag installed to ${BINDIR}"
echo "To use directly:"
echo "export PATH=${BINDIR}:\$PATH"

if [ "${1:-}" = "--activate" ]; then
    LINE="export PATH=${BINDIR}:\$PATH"

    if ! grep -qxF "$LINE" /root/.bashrc 2>/dev/null; then
        echo "$LINE" >> /root/.bashrc
        echo "Added PATH activation to /root/.bashrc"
    else
        echo "PATH activation already exists"
    fi
fi
