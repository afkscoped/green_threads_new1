# Changelog

## [Phase 17 - Stability Patch] - 2025-12-21
- **CRITICAL FIX**: Resolved Segmentation Faults in Web Dashboards (Ports 8080 & 9090).
  - **Fix 1 (Buffer Overflow)**: Fixed `snprintf` return value misuse in `src/dashboard.c` that allowed heapsmash when JSON buffer was full.
  - **Fix 2 (Race Condition)**: Replaced unsafe global static buffer in `examples/web_dashboard.c` with per-thread dynamic allocation.
  - **Fix 3 (Process Crash)**: Added `signal(SIGPIPE, SIG_IGN)` to `gthread_init` to prevent process termination when browsers close connections abruptly.
- **Verification**: Verified zero crashes on both dashboards using `curl` and interactive tests.
