# Changelog

## [Phase 14] - 2025-12-21
- **Feature**: Advanced Dashboard (Port 9090) embedded into interactive examples.
- **Feature**: Interactive Stride Scheduling Demo (Option 2) - Ask for thread count.
- **Feature**: Interactive Stack Demo (Option 3) - Ask for recursion depth.
- **Feature**: Interactive IO/Sleep Demos (Option 5, 6).
- **Core**: Added `src/dashboard.c` as a reusable module.
- **API**: Added `dashboard_start(port)` and `gthread_get_id()`.
- **Fix**: Resolved segmentation fault in dashboard server loop using non-blocking I/O.
