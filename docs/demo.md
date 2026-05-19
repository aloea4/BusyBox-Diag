# Demo Scenario

以下流程可直接給驗收者操作。

## Demo 1：BusyBox applet registration

```bash
./busybox --list | grep -E 'diag|ptop'
```

## Demo 2：diagfs

```bash
./busybox diagfs --output raw /
```

## Demo 3：diagnet

```bash
./busybox diagnet --output raw
```

## Demo 4：ptop

```bash
./busybox ptop --output table -1 -n 1
```

## Demo 5：BusyBox install dispatch

```bash
mkdir -p /tmp/busybox-diag
./busybox --install -s /tmp/busybox-diag
/tmp/busybox-diag/diagfs --help
```

說明：

`argv[0]` -> BusyBox applet dispatch -> `diagfs_main()`
