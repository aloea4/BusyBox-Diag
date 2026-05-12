# ptop — Snapshot-based Linux Process Monitoring Tool

ptop is a lightweight **snapshot-based process monitoring tool** for Linux systems, designed as part of the BusyBox-Diag diagnostic toolchain and built on top of `libdiag`.

Unlike traditional tools such as `top` or `htop` that continuously repaint live system state, ptop is built around a **deterministic snapshot + delta analysis model**, making it suitable for benchmarking, system diagnostics, and reproducible monitoring.

---

# 1. Design Philosophy

ptop follows a strict UNIX-style monitoring architecture:

