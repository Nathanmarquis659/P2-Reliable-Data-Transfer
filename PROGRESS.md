# PROGRESS.md

## Current Task
Task 2 — Receiver: implement the `expected`-counter state machine (§2.5). Test manually with the relay on a clean channel first.

## State
READY_FOR_REVIEW

## What's Done So Far
- **Task 1 — CLI is DONE** (commits 52705b9 → 348174a): `config_t` + `parse_args()` in src/lab.{h,c}, main.c wiring, 18 parser unit tests. (files touched: src/lab.h, src/lab.c, src/main.c, tests/lab-test.c)
- **Task 2 — receiver state machine + live clean-channel test DONE**: `src/rx.{h,c}` pure in-order receiver with 2 s FIN linger and 30 s idle give-up, 23 unit tests. Live relay runs completed in-sandbox 10/10 via user-approved `uv run cs425_relay.py`: clean channel → both exit 0, `cmp` identical; lossy `-l 0.2 -c 0.1 -d 0.2 --seed 7` → both exit 0, `cmp` identical (0.77 s vs 0.02 s clean, retransmissions observed).
- **Blocking-socket deadlock found and fixed during live testing**: the UDP socket was blocking, but both drain loops in src/main.c rely on non-blocking semantics (they break on EAGAIN); after the last ACK the sender blocked forever in `recvfrom` and never reached the FIN branch — relay log showed 5 DATA + 5 ACK then silence. Fix: `fcntl(F_SETFL, O_NONBLOCK)` in `open_relay()` (src/main.c). Verified via strace before/after; live clean+lossy runs pass after fix.
- **Task 3 — sender state machine DONE** (commits 7253e9c → 6d80839): `src/sx.{h,c}` per §2.4, one timer, window buffer seq % 64, give-up after 10 fruitless timeouts; 13 unit tests + 3 in-memory clean-channel e2e tests (w=1, w=16, empty file).
- **Task 4 — I/O glue DONE**: src/main.c holds the only OS-touching code; layers 1–2 confirmed I/O-free by grep. Live-relay runs of the glue completed in-sandbox via uv (see Task 2 line above).

## Next Immediate Step
Task 5 (next unchecked PLAN.md item): audit `tests/lab-test.c` against the required minimum cases — especially "end-to-end lossy test run with multiple seeds" — check it off if all present, or add missing cases.

## Open Questions / Blockers
- None for Task 2. Sandbox relay runs now work via `uv run cs425_relay.py` (user-approved this session).
- Onyx half of §1 environment gate is a user action (sandbox covers local build only).

## Last Known-Good State
Deadlock fix committed on top of I/O glue commit 97503b0: `make all` clean, `make check` 57/57 PASS, live relay clean + lossy transfers byte-identical through the real cs425_relay.py.
