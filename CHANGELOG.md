# Changelog

## [Phase 16] - 2025-12-21
- **Critical Fix**: Fixed segmentation fault / nothing displayed in Dashboard modules.
  - Resolved a buffer overflow/race condition in `dashboard.c` by switching from a static 128KB buffer to dynamic allocation per request.
  - Ensured `src/scheduler.c` poll logic is robust.
- **Verification**: Verified interactive `mutex_test.c` runs correctly with the dashboard.
