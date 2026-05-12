# ptop — Snapshot-based Linux Process Monitoring Tool

ptop is a lightweight **snapshot-based process monitoring tool** for Linux systems, designed as part of the BusyBox-Diag diagnostic toolchain and built on top of `libdiag`.

Unlike traditional tools such as `top` or `htop` that continuously repaint live system state, ptop is built around a **deterministic snapshot + delta analysis model**, making it suitable for benchmarking, system diagnostics, and reproducible monitoring.

---

# 1. Design Philosophy

ptop follows a strict UNIX-style monitoring architecture:

Linux /proc
↓
libdiag (measurement layer)
↓
snapshot engine
↓
delta analysis engine
↓
filter / sort / policy layer
↓
presentation layer (TTY / batch / JSON)


### Core principles

- Snapshot-based analysis (not continuous streaming UI)
- Strict separation of measurement / analysis / policy / presentation
- UNIX pipeline compatibility (filter → sort → output)
- Deterministic and reproducible results
- Compatible with Toybox / GNU-style CLI behavior
- Designed for embedding in BusyBox

---

# 2. Architecture Overview

## 2.1 Module structure


libdiag/
├── measurement layer (procfs parsing)

ptop/
├── snapshot.c → system snapshot collection
├── delta.c → CPU / memory delta computation
├── filter.c → query engine (pid/state/top-N filters)
├── tree.c → process hierarchy reconstruction
├── policy.c → sorting / ranking / top-N selection
├── render.c → ANSI TTY rendering
├── output.c → batch / JSON output
├── signal.c → terminal safety handling



---

# 3. Features

## 3.1 Process monitoring

- System-wide CPU usage (delta-based)
- Per-process CPU usage (utime/stime delta)
- Memory RSS tracking
- Process state tracking (R/S/D/Z)
- Full process hierarchy reconstruction

## 3.2 Query engine

Supported filters:

- PID filtering (`--pid`)
- Process state filtering (`--state`)
- Top-N limiting (`--top`)
- Sorting:
  - CPU usage
  - RSS memory
  - PID order

## 3.3 Output modes

- Interactive TTY mode (default)
- Batch mode (`--batch`)
- One-shot snapshot (`--once`)
- Pipe-friendly mode (auto-detect `isatty()`)

---

# 4. CLI Usage


ptop [OPTIONS]


## Options

| Option | Description |
|--------|------------|
| `-d SEC` | refresh interval (default: 1s) |
| `-n N` | run N iterations then exit |
| `-1` | single snapshot mode |
| `-b` | batch mode (no ANSI output) |
| `-t N` | show top N processes |
| `-s MODE` | sort mode: `pid`, `cpu`, `rss` |
| `-h` | show help |

---

# 5. Example Usage

## Interactive mode


ptop

## Top CPU processes

ptop -s cpu -t 10

## One-shot snapshot

ptop -1

## Batch mode (for scripts / CI / logging)

ptop -b -n 5

---

# 6. Filtering Model (UNIX pipeline design)

ptop applies a deterministic processing pipeline:

snapshot
↓
filter (pid / state)
↓
sort (cpu / rss / pid)
↓
policy (top-N selection)
↓
render / output


### Filter behavior

- PID filter: exact match
- State filter: match process state (R/S/D/Z)
- Top-N: truncates result set
- Sorting: deterministic qsort-based ordering

---

# 7. CPU Model

ptop uses a **snapshot delta CPU model**:

CPU% = process_delta_time / system_total_delta_time

Where:

- process_delta_time = utime + stime delta
- system_total_delta_time = /proc/stat total delta

### Characteristics

- Deterministic
- Reproducible
- Multi-core aware (not normalized per-core like GNU top)
- Suitable for benchmarking

---

# 8. Process Tree Model

ptop reconstructs process hierarchy using:

- PID → PPID mapping
- O(n) indexed reconstruction (no repeated scanning)

Example output:

init
├── systemd
│ ├── sshd
│ └── bash
└── kthreadd

---

# 9. Build Instructions

## 9.1 Build libdiag

make

Output:

libdiag.a

---

## 9.2 Build standalone ptop

make ptop

Binary:

tools/ptop/ptop

---

## 9.3 BusyBox integration

Enable configuration:

CONFIG_PTOP=y

Then build BusyBox:

make menuconfig
make

---

# 10. Testing

Run full test suite:

make test

Includes:

- process parsing tests
- filesystem diagnostics
- CPU snapshot validation
- error handling checks
- fixture-based tests

---

# 11. Performance Benchmarking

ptop is designed for benchmarking against standard UNIX tools:

| Tool | Purpose |
|------|--------|
| ps | process listing |
| top | CPU monitoring |
| htop | interactive monitoring |
| free | memory reporting |

### Benchmark target

- ≤ 50% overhead compared to GNU tools
- Linear `/proc` scanning
- Minimal syscall overhead
- No ncurses dependency in core engine

---

# 12. Portability Model

## POSIX-style CLI behavior

- stdout / stderr separation
- exit code semantics
- pipe-friendly output
- batch mode support

## Linux-specific backend

- `/proc/[pid]/stat`
- `/proc/[pid]/status`
- `/proc/stat`
- `/proc/meminfo`

---

# 13. Limitations

- Linux-only (requires procfs)
- No Windows support
- No full cgroup v2 integration yet
- No per-thread CPU breakdown yet
- RSS calculation simplified via `/proc/[pid]/status`

---

# 14. Future Work

## High priority

- cgroup-aware monitoring
- per-thread CPU tracking
- improved memory model (statm-based RSS)
- JSON schema stabilization

## Medium priority

- ncurses interactive UI
- remote process monitoring
- systemd integration
- container-aware process grouping

---

# 15. Relationship to Toybox / GNU tools

ptop is designed to be:

- Toybox-compatible in CLI behavior
- GNU ps/top-compatible in output semantics
- BusyBox-compatible in embedding model

However:

> ptop is NOT a drop-in replacement for top/htop.

It is a **deterministic snapshot-based process analysis engine**.

---

# 16. Signal Safety

ptop includes:

- SIGINT handling (Ctrl+C cleanup)
- terminal state restoration
- cursor visibility control
- safe exit via `atexit()`

---

# 17. Output Philosophy

ptop supports multiple output styles:

- Interactive ANSI TTY rendering
- Batch machine-readable output
- Pipe-friendly mode (auto-detected)

---

# 18. Authoring Model

ptop is designed as:

> A snapshot-based process analysis engine, not a UI tool.

---

# 19. License

MIT License (or BusyBox-compatible module license depending on integration target)

---

# 20. Summary

ptop implements:

- Snapshot-based system monitoring
- Delta-based CPU computation
- UNIX-style filtering pipeline
- Toybox-compatible CLI semantics
- BusyBox applet integration

It is designed for:

- system diagnostics
- embedded Linux environments
- performance benchmarking
- educational OS/process monitoring architecture
