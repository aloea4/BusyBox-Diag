# NAME

diagfs \- Filesystem metadata monitoring tool

# SYNOPSIS

diagfs \[OPTIONS\] [PATH]

# DESCRIPTION

`diagfs` inspects filesystem metadata for a target path (default `/`). It reports capacity, inode usage, and optional FIEMAP-derived extent observations. It is designed for lightweight system diagnostics, not recursive file-content analysis.

# OPTIONS

- `--output table|raw|json`  
  Select output format.
- `--color auto|always|never`  
  Set color rendering mode for human-readable output.
- `--quiet`  
  Suppress non-fatal warning messages.
- `--no-header`  
  Disable header lines in table/raw output.
- `--all`  
  Scan all mounted filesystems.
- `--real` / `--pseudo`  
  Filter mount classes when `--all` is used.
- `--fiemap`  
  Probe extent layout with FIEMAP.
- `-h`, `--help`  
  Show help.

# OUTPUT MODES

- `table`: human-friendly summary view
- `raw`: pipeline-friendly fields for shell tools
- `json`: machine-readable structured output

# EXAMPLES

```bash
diagfs --output table /
diagfs --output raw /
diagfs --output json /
diagfs --all --real --output table
```

# DESIGN NOTES

`diagfs` focuses on metadata observability. It does not recursively calculate total directory size, and extent observations should be interpreted as lightweight signals rather than a full fragmentation scoring system.
