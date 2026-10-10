# PROGRESS.md

## Current Task
Task 4 — Testable design (critical, explicitly graded): split `src/lab.h` into exactly three layers — Packets (pure), State machines (no I/O), I/O glue (only layer touching the OS).

## State
READY_FOR_REVIEW

## What's Done So Far
- **Task 1 — CLI is DONE** (commits 52705b9 → 348174a): `config_t` + `parse_args()` in src/lab.{h,c}, main.c wiring, 18 parser unit tests. (files touched: src/lab.h, src/lab.c, src/main.c, tests/lab-test.c)
- **Task 2 — packet layer + receiver state machine** (src/pkt.{h,c}, src/rx.{h,c}): RFC 1071 checksum, manual byte-order encode, ordered validate; pure in-order receiver with 2 s FIN linger and 30 s idle give-up. 23 unit tests. Manual clean-channel relay test still pending (see Blockers).
- **Task 3 — sender state machine DONE** (commits 7253e9c → 6d80839): `src/sx.{h,c}` per §2.4, one timer, window buffer seq % 64, give-up after 10 fruitless timeouts; 13 unit tests + 3 in-memory clean-channel e2e tests (w=1, w=16, empty file).
- **Task 4 — I/O glue DONE**: src/main.c now holds the only OS-touching code: getaddrinfo UDP socket (one per run), HELLO/OK handshake (5 attempts × 1 s wait; OK→0, ERR→2, silence→retry), sender poll loop with timer-remaining timeout + recvfrom batch driving sx_on_packet/sx_tick, receiver poll loop keyed to linger/idle deadline + in-order fwrite and cumulative ACK sends, CLOCK_MONOTONIC ms clock. Confirmed layers 1–2 are I/O-free (grep for sendto/recvfrom/poll/clock_gettime/fopen/fread/fwrite/socket/getaddrinfo across pkt/sx/rx: zero hits). Verified: `make all` clean (4 variants), `make check` 57/57 PASS, `make leak` and `make leak-test` exit 0, no-relay runs of both modes exit 2 after 5 handshake attempts. (files touched: src/main.c)
- Live relay integration (clean + lossy channel) is BLOCKED in this sandbox — see Blockers for the exact commands to run outside it.

## Next Immediate Step
Task 2's manual clean-channel relay test (top unchecked PLAN.md item) requires a live relay run, which the sandbox denies; awaiting human triage — run the commands under Blockers or approve one.

## Open Questions / Blockers
- Live relay runs are blocked in the sandbox: every attempt to start `cs425_relay.py` (via uv or python3, backgrounded or fully self-contained script) was denied by the OpenCode external_directory permission rules. To unblock, run from a normal shell (repo root):
  - Clean channel: `python3 cs425_relay.py --delay 50 &`, then `./build/release/myapp recv -s t1 127.0.0.1 out.bin &`, then `./build/release/myapp send -s t1 127.0.0.1 in.bin`; expect both exit 0 and `cmp in.bin out.bin` to match.
  - Lossy: relay with `--seed 42`, sender adds `-l 0.2 -c 0.2 -d 0.2`.
- Onyx half of §1 environment gate is a user action (sandbox covers local build only).

## Last Known-Good State
I/O glue committed: `make all` clean, `make check` 57/57 PASS, `make leak`/`make leak-test` exit 0; live relay transfer not yet run (see Blockers).
