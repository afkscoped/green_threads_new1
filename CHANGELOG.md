# Changelog

## [Phase 15] - 2025-12-21
- **Fix**: Resolved segmentation fault in Dashboard/IO integration by preserving thread structs in zombie collection (safe iteration for dashboard).
- **Fix**: Removed redundant `gthread_yield` in `src/io.c` which caused unnecessary context switches.
- **Feature**: Enhanced Advanced Dashboard (Port 9090) with 5 dedicated sections:
  1. Stride Scheduling (Tickets/Pass)
  2. Stack Management (Usage Visuals)
  3. Synchronization (Mutex Blocked Threads)
  4. Sleep & Timers (Wake Times)
  5. I/O Integration (Blocked FDs)
- **Feature**: Interactive Synchronization Demo (`mutex_test.c`).
- **Feature**: Added `wake_time` field to JSON API for sleep visualization.
- **UI**: Added dynamic filtering and dedicated panels in `dashboard.js` and `index.html`.
