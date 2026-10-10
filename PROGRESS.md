# PROGRESS.md

## Current Task
Task 7 — Coverage (DONE this session): `make clean && make all && make report` → **100% coverage, 68/68 tests passing.**

## State
READY_FOR_REVIEW

## What's Done So Far
- **Tasks 1–6 DONE** (see git log). Task 6 committed: README has Results table + analysis.
- **Task 7 DONE**: `make clean && make all` clean; added 9 tests to `tests/lab-test.c` covering the only uncovered lines (all in `src/lab.c` `parse_args` error paths): `-T`/`-c` nonnumeric, `-c`/`-d` on recv, unknown option `-x`, and options missing their value. **Glibc getopt quirk found**: after a sequence of calls, an option missing its argument yields `'?'` (→ default case), not the option char with NULL `optarg` as on a fresh process — so the NULL guards in `parse_int`/`parse_double` are only reachable via a test-only seam: added `lab_test_parse_int`/`lab_test_parse_double` wrappers under `-DTEST` in `src/lab.c` + 2 direct NULL tests. Result: `make report` → **TOTAL 330/330 = 100%** (pkt/rx/sx were already 100%; no exclusions used).
- Task 6 raw data rescued from `build/` (wiped by `make clean`) to repo root: `t6-results.tsv`, `relay-t6.log` (untracked; `/tmp/opencode` blocked in this session too).
- **Task 6 DONE**: created `1mib.bin` (1 MiB urandom); one relay instance (`uv run cs425_relay.py --delay 50`, log `build/relay-t6.log`) served all runs with unique sessions `t6-w{W}-l{L}-r{R}`; release build timed via `date +%s.%N` around `myapp send` only. **12/12 runs: exit 0 both sides, `cmp` byte-identical** (raw data in `build/t6-results.tsv`, gitignored). Means: w1/l0 103.538 s (9.9 KiB/s), w16/l0 6.604 s (155.1 KiB/s), w1/l5 133.104 s (7.7 KiB/s), w16/l5 24.882 s (41.2 KiB/s). RTT implied by w1/l0 ≈ 101.0 ms vs 100 ms configured; speedup 15.7×; loss penalty +29% (w1) vs +277% (w16). Results table + 3-part analysis added to README.md; relay stopped, port 4250 free.

## Next Immediate Step
Task 8 (next unchecked PLAN.md item): `make leak` then `make leak-test`; fix every leak/crash, checking retransmission buffers and error paths specifically.

## Open Questions / Blockers
- None. Note: `/tmp/opencode` is blocked in this sandbox (permission rules deny it even though global docs say pre-approved) — keep working files inside the repo (gitignored `build/` or root). Backlog: uncommitted deletion of `submission-report.md` from a prior `make clean` still in worktree; new untracked `t6-results.tsv` + `relay-t6.log` at repo root — human to decide commit/restore.

## Last Known-Good State
Task 7 committed: `make clean && make all` clean (all 4 builds), `make check` 68/68 PASS, `make report` TOTAL 330/330 = 100% line coverage (no exclusions beyond the target's built-in main.c/test/harness excludes).
