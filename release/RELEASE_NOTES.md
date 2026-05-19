# BusyBox-Diag Release Notes

## Version

- BusyBox-Diag: v1.0-phase1
- BusyBox base: 1.36.1

## Integrated applets

- diagfs
- diagnet
- ptop

## Completed verification

- BusyBox integration build PASS
- Reproducible build script PASS
- Compatibility/efficiency verification PASS
- Applet dispatch PASS
- raw/json/table output PASS
- JSON validation PASS

## Known limitations

- Baseline network tool (`ss`/`netstat`) availability depends on host environment.
- `/usr/bin/time` may be unavailable in minimal containers.
- Existing compiler warnings are retained by design in this phase.
