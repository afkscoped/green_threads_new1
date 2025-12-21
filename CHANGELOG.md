# Changelog

## [Phase 18 - Advanced Dashboard Restoration] - 2025-12-21
- **Feature Restored**: Re-enabled the Advanced Dashboard (Port 9090) with full metrics (Tickets, Pass, Strides, Stack Usage, I/O Waiting).
- **Security Hardening**:
  - Implemented safe stack traversal in `runtime_get_stack_stats` to prevent reading terminated/zombie threads.
  - Retained the critical `snprintf` buffer overflow protection in `dashboard.c`.
  - Verified `web_dashboard` (Port 8080) uses thread-safe memory allocation.
- **Simplification**: Removed complex auto-launching of dashboards from basic demos to improve focus and stability.
- **Status**: Both dashboards now run simultaneously and stably.

## [Phase 17 - Stability Patch] - 2025-12-21
- **CRITICAL FIX**: Resolved Segmentation Faults in Web Dashboards (Ports 8080 & 9090).
  - **Fix 1 (Buffer Overflow)**: Fixed `snprintf` return value misuse in `src/dashboard.c` that allowed heapsmash when JSON buffer was full.
  - **Fix 2 (Race Condition)**: Replaced unsafe global static buffer in `examples/web_dashboard.c` with per-thread dynamic allocation.
  - **Fix 3 (Process Crash)**: Added `signal(SIGPIPE, SIG_IGN)` to `gthread_init` to prevent process termination when browsers close connections abruptly.
- **Verification**: Verified zero crashes on both dashboards using `curl` and interactive tests.
