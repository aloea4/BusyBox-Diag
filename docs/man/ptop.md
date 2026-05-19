# NAME

ptop \- Snapshot-based lightweight process monitor

# SYNOPSIS

ptop \[OPTIONS\]

# DESCRIPTION

`ptop` collects process and system snapshots from procfs, computes delta metrics, and renders lightweight process views suitable for both terminal and script use.

# SNAPSHOT-DELTA ARCHITECTURE

`ptop` operates in cycles:

1. Measurement snapshot collection
2. Delta computation between snapshots
3. Policy/filter/sort application
4. Output rendering

This design enables low-overhead CPU/memory trend reporting without requiring a full ncurses clone.

# OPTIONS

- `-1`  
  One-shot mode.
- `-n COUNT`  
  Refresh COUNT times then exit.
- `-t N`  
  Show top N processes.
- `--output table|raw|json`  
  Select output format.
- `-s pid|cpu|rss`  
  Select sort mode.
- `-p PID`  
  Filter by pid.
- `-S STATE`  
  Filter by process state.
- `-h`, `--help`  
  Show help.

# OUTPUT MODES

- `table`: terminal-readable summary
- `raw`: pipeline/script-friendly rows
- `json`: structured integration output

# EXAMPLES

```bash
ptop --output table -1 -t 10
ptop --output raw -1 -n 5
ptop --output json -1 -n 1
```

# DESIGN NOTES

`ptop` follows layered separation:

- measurement: procfs snapshot collection
- policy: filtering/sorting/top-N selection
- output: table/raw/json rendering
