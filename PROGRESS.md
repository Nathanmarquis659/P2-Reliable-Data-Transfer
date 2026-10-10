# PROGRESS.md

## Current Task
Task 1 — CLI: Implement argument parsing exactly as specified above; verify both modes, all flags, defaults, and exit codes.

## State
IN_PROGRESS

## What's Done So Far
- Baseline gate passed: `make clean && make all && make check` green on starter code (debug/release/test/debug-test all compile under strict `-Werror=...` flags; starter test passes). (files touched: none)
- Added `config_t` + `parse_args()` to src/lab.h / src/lab.c. Parser handles mode, all flags, defaults, ranges, session charset, and exactly-two-positionals; returns exit code 0/1. Renamed enum to `app_mode_t` (POSIX `mode_t` collision via `<netdb.h>`). Kept placeholder get_greeting so build stays green. Verified: `make all` clean, zero warnings/errors. (files touched: src/lab.h, src/lab.c)
- Wired src/main.c to call parse_args; prints effective config on success, returns exit code. Smoke-tested exit-code matrix (no-args→0, valid send/recv→0, bad mode / out-of-range -w / rate>0.5 / missing-or-extra positional / send-flag-on-recv / bad session charset →1). Also committed cs425_relay.py (valid 383-line Python) for later tasks. (files touched: src/main.c, cs425_relay.py)
- Replaced tests/lab-test.c placeholder with 18 CLI parser unit tests (no-args, valid send/recv field checks, bad mode, -w bounds/nonnumeric, rate bounds, port bound, missing/extra positional, send-flag-on-recv, session length/charset, missing -s). Removed get_greeting from lab.h/lab.c. Added `-s` required check to parse_args (spec shows it unbracketed = mandatory). Double comparison via local `assert_double_near` helper (Unity's double asserts need UNITY_INCLUDE_DOUBLE at unity.c compile time, which the Makefile doesn't set). Verified: `make all` clean (test bin builds), `make check` 18/18 PASS, `make leak` exit 0 (ASan debug build), runtime exit-code matrix correct. (files touched: tests/lab-test.c, src/lab.c, src/lab.h)

## Next Immediate Step
Final step for Task 1: check off Task 1 in PLAN.md, set PROGRESS.md State→READY_FOR_REVIEW with Next Immediate Step pointing at Task 2, then commit and stop.

## Open Questions / Blockers
- Relay (`cs425_relay.py`) not downloaded yet — not needed for Task 1; required later for manual integration testing and Task 6 timing.
- Onyx half of §1 environment gate is a user action (this sandbox covers the local build only).

## Last Known-Good State
Task 1 CLI complete: `make all` clean, `make check` 18/18 PASS, `make leak` exit 0. Working tree has uncommitted parser-test changes (src/lab.c, src/lab.h, tests/lab-test.c) + PROGRESS.md pending final commit.
