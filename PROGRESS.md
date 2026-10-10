# PROGRESS.md

## Current Task
Task 3 — Sender: Implement the `base`/`next`/timer state machine (§2.4). Verify stop-and-wait (`-w 1`) and GBN (`-w 16`) both behave correctly.

## State
IN_PROGRESS

## What's Done So Far
- **Task 1 — CLI is DONE** (commits 52705b9 → 348174a): `config_t` + `parse_args()` in src/lab.{h,c}, main.c wiring, 18 parser unit tests, `-s` required. Verified: `make all` clean, `make check` 18/18 PASS, `make leak` exit 0. (files touched: src/lab.h, src/lab.c, src/main.c, tests/lab-test.c)
- Task 2 — packet layer added (src/pkt.{h,c}): RFC 1071 checksum, manual byte-order encode, ordered validate per §2.3; 12 unit tests incl. §2.6 wire-byte fixture and RFC 1071 example. (files touched: src/pkt.h, src/pkt.c, tests/lab-test.c)
- Task 2 — receiver state machine added (src/rx.{h,c}): pure struct per §2.5, fake-clock driven, actions NONE/ACK/WRITE_ACK/DONE/GIVEUP, 2 s FIN linger, 30 s idle give-up; 11 unit tests. Verified: `make all` clean, `make check` 41/41 PASS, `make leak` exit 0. (files touched: src/rx.h, src/rx.c, tests/lab-test.c)
- Task 3 — agreed scope with human: unit-level verification (incl. in-memory e2e at w=1/w=16) satisfies the task; live relay runs of `-w 1`/`-w 16` happen during Task 4 integration. Task 3 gets checked off when `make all`/`check`/`leak` pass.

## Next Immediate Step
Write src/sx.h + src/sx.c (sender state machine: init/on_packet/tick/next_send/status/timer_remaining), then `make all`.

## Open Questions / Blockers
- Task 2 manual clean-channel relay test deferred until I/O glue (Task 4 layer 3) exists.
- Onyx half of §1 environment gate is a user action (sandbox covers local build only).

## Last Known-Good State
Receiver state machine + packet layer committed (fae8e1e): `make all` clean, `make check` 41/41 PASS, `make leak` exit 0; working tree clean.
