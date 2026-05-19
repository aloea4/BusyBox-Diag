# Local Install Guide

## Install

```bash
scripts/install_busybox_diag_local.sh
```

## Activate PATH

```bash
export PATH=/usr/local/busybox-diag/bin:$PATH
```

## Persistent activation

```bash
scripts/install_busybox_diag_local.sh --activate
source /root/.bashrc
```

## Test

```bash
diagfs --output table /
diagnet --output raw
ptop --output table -1 -n 1
```

## Uninstall

```bash
rm -rf /usr/local/busybox-diag
```
