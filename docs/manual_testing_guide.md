# BusyBox-Diag Manual Testing Guide

## 1. Test Environment

- Project root: `/root/workspace/busybox-1.36.1`
- Binary under test: `./busybox`
- Integrated applets: `diagfs`, `diagnet`, `ptop`
- Python JSON validator: `python3 -m json.tool`

## 2. Basic Applet Registration Test

Run:

```bash
./busybox --list | grep -E 'diagfs|diagnet|ptop'
./busybox diagfs --help
./busybox diagnet --help
./busybox ptop --help
```

Expected:

- All three applets appear in `--list`
- Help output returns successfully for all three applets

## 3. diagfs Manual Tests

Run:

```bash
./busybox diagfs --output table /
./busybox diagfs --output raw /
./busybox diagfs --output json / | python3 -m json.tool
```

Expected:

- Table output shows filesystem usage summary
- Raw output is single-line/script-friendly
- JSON parses successfully and includes usage/inode/layout fields

## 4. diagnet Manual Tests

Run:

```bash
./busybox diagnet --output table
./busybox diagnet --output raw
./busybox diagnet --output json | python3 -m json.tool
```

Expected:

- Table output shows TCP/UDP metadata rows
- Raw output is line-oriented and pipeline-friendly
- JSON parses successfully with `connections` and `summary`

## 5. ptop Manual Tests

Run:

```bash
./busybox ptop --output table -1 -n 1
./busybox ptop --output raw -1 -n 5
./busybox ptop --output json -1 -n 1 | python3 -m json.tool
```

Expected:

- One-shot snapshot works (`-1 -n 1`)
- Raw mode emits machine-friendly records
- JSON parses successfully and contains timestamp/system/process fields

## 6. BusyBox Symlink Dispatch Test

Run:

```bash
rm -rf /tmp/busybox-diag-demo
mkdir -p /tmp/busybox-diag-demo
./busybox --install -s /tmp/busybox-diag-demo
/tmp/busybox-diag-demo/diagfs --help
/tmp/busybox-diag-demo/diagnet --output raw
/tmp/busybox-diag-demo/ptop --output table -1 -n 1
```

Expected:

- Symlink-invoked commands run correctly
- Confirms dispatch path:

`argv[0] -> BusyBox applet dispatch -> xxx_main()`

## 7. Pipeline / Script-friendly Output Test

Run:

```bash
./busybox diagfs --output raw / | awk '{print $1, $2, $3}'
./busybox diagnet --output raw | head -20
./busybox ptop --output raw -1 -n 10 | head -20
```

Expected:

- Outputs can be piped into `awk`, `head`, and shell tooling without format breakage

## 8. Automated Verification Scripts

Run:

```bash
scripts/build_busybox_diag.sh
scripts/verify_busybox_diag.sh
bash demo/run_demo.sh
```

Expected:

- All scripts exit 0
- Verification script ends with `BusyBox-Diag verification PASSED`
- Demo script ends with `BusyBox-Diag demo PASSED`

## 9. Expected Results

- Applet registration: PASS
- Help checks: PASS
- Table/raw/json output: PASS
- JSON parse validation: PASS
- Symlink dispatch behavior: PASS
- Pipeline friendliness: PASS
- Automated scripts: PASS

## 10. Known Environment-dependent SKIPs

- `ss` / `netstat` may be unavailable in minimal container environments
- `/usr/bin/time` may be unavailable; timing checks may be skipped
- Connection/process counts vary by runtime environment and are not strict-equality checks
