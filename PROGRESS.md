# PROGRESS.md

## Current Task
Task 1 — CLI: Implement argument parsing exactly as specified above; verify both modes, all flags, defaults, and exit codes.

## State
IN_PROGRESS

## What's Done So Far
- Baseline gate passed: `make clean && make all && make check` green on starter code (debug/release/test/debug-test all compile under strict `-Werror=...` flags; starter test passes). (files touched: none)

## Next Immediate Step
Add `config_t` struct and `parse_args()` to src/lab.h + src/lab.c (keep placeholder get_greeting for now so the build stays green), then commit.

## Open Questions / Blockers
- Relay (`cs425_relay.py`) not downloaded yet — not needed for Task 1; required later for manual integration testing and Task 6 timing.
- Onyx half of §1 environment gate is a user action (this sandbox covers the local build only).

## Last Known-Good State
Starter code builds clean and `make check` passes (commit 4c5ebda, plus uncommitted PLAN/PROGRESS/AGENTS files).
