# PROGRESS.md

## Current Task
Task 6 — Measure the window:

## State
IN_PROGRESS

## What's Done So Far
- **Tasks 1–5 DONE** (see git log). `make all` clean, `make check` 59/59 PASS, live relay transfers byte-identical.
- Task 6 plan approved by user: release build, one relay (`--delay 50`) for all runs, unique session per run, time = wall-clock of `myapp send` only, 12 runs total (4 combos × 3), smoke run first.

## Next Immediate Step
Prep: `head -c 1048576 /dev/urandom > 1mib.bin`, add `1mib.bin`/`in.bin`/`out.bin` to .gitignore, commit; then start relay and do the smoke run (w=16, loss=0).

## Open Questions / Blockers
- None. (Backlog note: uncommitted deletion of submission-report.md from a prior `make clean` — left untouched, human triage.)

## Last Known-Good State
Task 5 committed: `make all` clean, `make check` 59/59 PASS, live relay clean + lossy transfers byte-identical through the real cs425_relay.py.
