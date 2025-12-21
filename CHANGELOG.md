# Changelog

## [Phase 17] - 2025-12-21
- **CRITICAL FIX**: Resolved Segmentation Faults in Dashboard (Options 2-10).
  - Cause: Uninitialized `waiting_fd` and `wake_time_ms` fields in `gthread_t` led to garbage values.
  - Fix: Added proper initialization in `gthread_create` and `gthread_init`.
  - Fix: Added robust boundary checks in `runtime_get_stack_stats` to prevent invalid pointer access on terminated threads.
- **Verification**: Verified stability of `stride_test` and `web_dashboard` with active stack monitoring.
