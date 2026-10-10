# PROGRESS.md

## Current Task
Task 2 — Receiver: Implement the `expected`-counter state machine (§2.5). Test manually with the relay on a clean channel first.

## State
IN_PROGRESS

## What's Done So Far
- **Task 1 — CLI is DONE** (commits 52705b9 → 348174a): `config_t` + `parse_args()` in src/lab.{h,c}, main.c wiring, 18 parser unit tests, `-s` required. Verified: `make all` clean, `make check` 18/18 PASS, `make leak` exit 0; working tree clean. (files touched: src/lab.h, src/lab.c, src/main.c, tests/lab-test.c)
- Task 2 — packet layer added (src/pkt.{h,c}): RFC 1071 checksum, manual byte-order encode, ordered validate per §2.3 (length checked vs actual size before payload is touched); 12 unit tests incl. §2.6 wire-byte fixture (`"Hi!"` seq=2) and RFC 1071 example. Verified: `make all` clean, `make check` 30/30 PASS, `make leak` exit 0. (files touched: src/pkt.h, src/pkt.c, tests/lab-test.c)

## Next Immediate Step
Add the receiver state machine (src/rx.{h,c}) per §2.5 — pure struct, takes now_ms, returns actions (NONE/ACK/WRITE_ACK/DONE/GIVEUP), 2 s FIN linger, 30 s idle give-up — plus ~11 Unity tests incl. the 2500-byte worked example; verify and commit.

## Open Questions / Blockers
- Task 2 manual clean-channel relay test deferred until I/O glue (Task 4 layer 3) + sender (Task 3) exist; consistent with §7 ordering.
- `cs425_relay.py` is present (committed) but not yet used — needed for that manual test + Task 6 timing.
- Onyx half of §1 environment gate is a user action (sandbox covers local build only).

## Last Known-Good State
Packet layer committed: `make all` clean, `make check` 30/30 PASS, `make leak` exit 0; working tree clean.
