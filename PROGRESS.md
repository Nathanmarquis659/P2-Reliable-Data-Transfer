# PROGRESS.md

## Current Task
Task 1 — CLI: Implement argument parsing exactly as specified above; verify both modes, all flags, defaults, and exit codes.

## State
IN_PROGRESS

## What's Done So Far
- Baseline gate passed: `make clean && make all && make check` green on starter code (debug/release/test/debug-test all compile under strict `-Werror=...` flags; starter test passes). (files touched: none)
- Added `config_t` + `parse_args()` to src/lab.h / src/lab.c. Parser handles mode, all flags, defaults, ranges, session charset, and exactly-two-positionals; returns exit code 0/1. Renamed enum to `app_mode_t` (POSIX `mode_t` collision via `<netdb.h>`). Kept placeholder get_greeting so build stays green. Verified: `make all` clean, zero warnings/errors. (files touched: src/lab.h, src/lab.c)

## Next Immediate Step
Wire src/main.c entry point to call parse_args (print effective config on success, return exit code), then commit.

## Open Questions / Blockers
- Relay (`cs425_relay.py`) not downloaded yet — not needed for Task 1; required later for manual integration testing and Task 6 timing.
- Onyx half of §1 environment gate is a user action (this sandbox covers the local build only).

## Last Known-Good State
Starter code builds clean and `make check` passes (commit 4c5ebda, plus uncommitted PLAN/PROGRESS/AGENTS files).
