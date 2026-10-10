# PROGRESS.md

## Current Task
Task 6 — Measure the window:

## State
READY_FOR_REVIEW

## What's Done So Far
- **Tasks 1–5 DONE** (see git log). `make all` clean, `make check` 59/59 PASS.
- **Task 6 DONE**: created `1mib.bin` (1 MiB urandom); one relay instance (`uv run cs425_relay.py --delay 50`, log `build/relay-t6.log`) served all runs with unique sessions `t6-w{W}-l{L}-r{R}`; release build timed via `date +%s.%N` around `myapp send` only. **12/12 runs: exit 0 both sides, `cmp` byte-identical** (raw data in `build/t6-results.tsv`, gitignored). Means: w1/l0 103.538 s (9.9 KiB/s), w16/l0 6.604 s (155.1 KiB/s), w1/l5 133.104 s (7.7 KiB/s), w16/l5 24.882 s (41.2 KiB/s). RTT implied by w1/l0 ≈ 101.0 ms vs 100 ms configured; speedup 15.7×; loss penalty +29% (w1) vs +277% (w16). Results table + 3-part analysis added to README.md; relay stopped, port 4250 free.

## Next Immediate Step
Task 7 (next unchecked PLAN.md item): `make clean && make all && make report` → drive to 100% coverage with all tests passing (only system/library-call branches may be excluded).

## Open Questions / Blockers
- None. Note: this session's permission rules blocked all file access under `/tmp/opencode` (even the Write tool), so working files went in gitignored `build/` instead; a future session should assume the same unless told otherwise. (Backlog: uncommitted deletion of submission-report.md from a prior `make clean` — left untouched, human triage.)

## Last Known-Good State
Task 6 committed: README has Results table + analysis; PLAN.md Task 6 checked off; `build/t6-results.tsv` holds raw per-run times. No source code changed by Task 6.
