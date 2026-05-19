# NAME

diagnet \- Passive kernel socket state observer

# SYNOPSIS

diagnet \[OPTIONS\]

# DESCRIPTION

`diagnet` is **not** an active socket client/server tool. It is a passive kernel socket state observer that reads exported kernel connection metadata and summarizes TCP/UDP connection state.

# KERNEL SOCKET TABLE

`diagnet` reads:

- `/proc/net/tcp`
- `/proc/net/udp`

These procfs views reflect the kernel socket table state at read time. `diagnet` does not initiate remote probes and does not capture packets.

# OPTIONS

- `--output table|raw|json`  
  Select output format.
- `--quiet`  
  Suppress non-fatal warning messages.
- `--no-header`  
  Disable headers in table/raw output.
- `--proto tcp|udp|all`  
  Filter protocol set.
- `--state STATE` / `--listen`  
  Filter TCP state.
- `--local-port PORT`, `--remote-port PORT`  
  Filter by endpoint port.
- `--local-addr ADDR`, `--remote-addr ADDR`  
  Filter by endpoint address.
- `--suspicious`, `--whitelist PORTS`  
  Apply suspicious-listen heuristic controls.
- `-h`, `--help`  
  Show help.

# OUTPUT MODES

- `table`: human-readable connection table
- `raw`: line-oriented shell-friendly records
- `json`: structured output for automation

# EXAMPLES

```bash
diagnet --output table
diagnet --output raw
diagnet --output json
diagnet --proto tcp --state ESTABLISHED --output raw
```

# DESIGN NOTES

The tool is optimized for lightweight operational diagnostics using existing kernel-exported metadata rather than active network interactions.
