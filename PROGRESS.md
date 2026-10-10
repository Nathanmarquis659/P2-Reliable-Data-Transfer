# PROGRESS.md

## Current Task
Task 3 — Sender: Implement the `base`/`next`/timer state machine (§2.4). Verify stop-and-wait (`-w 1`) and GBN (`-w 16`) both behave correctly.

## State
READY_FOR_REVIEW

## What's Done So Far
- **Task 1 — CLI is DONE** (commits 52705b9 → 348174a): `config_t` + `parse_args()` in src/lab.{h,c}, main.c wiring, 18 parser unit tests, `-s` required. Verified: `make all` clean, `make check` 18/18 PASS, `make leak` exit 0. (files touched: src/lab.h, src/lab.c, src/main.c, tests/lab-test.c)
- Task 2 — packet layer added (src/pkt.{h,c}): RFC 1071 checksum, manual byte-order encode, ordered validate per §2.3; 12 unit tests incl. §2.6 wire-byte fixture and RFC 1071 example. (files touched: src/pkt.h, src/pkt.c, tests/lab-test.c)
- Task 2 — receiver state machine added (src/rx.{h,c}): pure struct per §2.5, fake-clock driven, actions NONE/ACK/WRITE_ACK/DONE/GIVEUP, 2 s FIN linger, 30 s idle give-up; 11 unit tests. (files touched: src/rx.h, src/rx.c, tests/lab-test.c)
- **Task 3 — sender state machine DONE** (commits 7253e9c → 6d80839): `src/sx.{h,c}` per §2.4 — `base`/`next`, one timer, window buffer indexed by seq % 64, lazy encode from borrowed file pointer; FIN sent when `base == n_data` (seq = n_data, empty file → FIN seq 0), retransmitted on timeout until ACKed; give-up after 10 consecutive timeouts with no base movement. 13 unit tests + 3 in-memory clean-channel e2e tests (w=1 × 2500 B, w=16 × 20480 B, empty file) driving sx↔rx through zero loss and asserting bytes-out == bytes-in. Verified: `make all` clean, `make check` 57/57 PASS, `make leak-test` exit 0. (files touched: src/sx.h, src/sx.c, tests/lab-test.c)
- Task 3 scope note (agreed with human): unit-level verification satisfies the task; live relay runs of `-w 1`/`-w 16` happen during Task 4 integration.

## Next Immediate Step
Task 4 — Testable design: split into three layers (packets / state machines / I/O glue). Packets (pkt.{h,c}) and both state machines (sx.{h,c}, rx.{h,c}) are already pure and I/O-free; the remaining work is the thin I/O layer in main.c (socket, HELLO/OK handshake with retries, poll loop with timer-remaining timeout, file read/write) plus confirming no I/O calls leak into layers 1–2.

## Open Questions / Blockers
- Task 2 manual clean-channel relay test deferred until I/O glue (Task 4 layer 3) exists.
- Onyx half of §1 environment gate is a user action (sandbox covers local build only).

## Last Known-Good State
Sender state machine committed (6d80839): `make all` clean, `make check` 57/57 PASS, `make leak-test` exit 0; working tree clean.
