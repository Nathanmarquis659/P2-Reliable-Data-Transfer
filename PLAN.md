# P2 – Reliable Data Transfer: Agent Execution Plan

**Course:** CS 425 · **Points:** 100 · **Due:** 10/11/2026 · **Submission:** `submission-report.docx` generated via GitHub Action, uploaded to Canvas

## 0. Assignment in One Paragraph

Build a single program, `myapp`, with two modes (`send` and `recv`), that reliably transfers a file over UDP through a lossy/corrupting/duplicating relay. The reliability mechanism is **Go-Back-N** (rdt3.0 is just GBN with window=1). You implement a custom packet format with checksums and sequence numbers, a sender sliding-window state machine with one retransmission timer, and a simple in-order receiver. The protocol logic must be unit-testable without real sockets/clocks. Must compile on **both GitHub Codespaces and Onyx** (zero credit otherwise) and pass through CI producing a submission report.

---

## 1. Environment Setup

1. Fork `https://github.com/shanep/makefile-project-starter` via "Use this template." Name the copy `cs425-p2`.
2. Clone locally / open in Codespaces.
3. Download the relay: `cs425_relay.py` (single Python 3 file, no dependencies) from the Canvas assignment page.
4. Confirm `make`, `make check`, `make leak`, `make report` targets exist in the starter and run cleanly before any changes.
5. Verify the project builds on **both** target environments before writing protocol code — this is a hard zero-point gate, not a nice-to-have.

---

## 2. Core Concept Translation (from diagrams → explicit rules)

### 2.1 System topology (3 processes)
- `myapp send`: reads `in.bin`, splits into 1024-byte chunks, keeps a window of unacked packets in flight, runs **one** retransmission timer.
- `myapp recv`: writes `out.bin` strictly in order, ACKs every validated packet it receives.
- `cs425_relay.py`: a UDP relay on port 4250 (configurable) that independently, per datagram, per direction: (1) drops with prob. `loss`, else (2) corrupts 1 bit with prob. `corrupt`, else (3) duplicates with prob. `dup`. HELLO/OK handshake messages are never damaged.

### 2.2 Registration handshake (must happen before any data)
```
receiver -> relay: HELLO <session> recv
sender   -> relay: HELLO <session> send <loss> <corrupt> <dup>
relay    -> each:  OK   (or ERR <reason>)
```
- `<session>`: 1–32 chars, `[a-z0-9-]`, same string both sides use (e.g. BroncoName + suffix).
- Rates are decimals in `[0, 0.5]`.
- **Receiver must register first** — sender's hello gets `ERR no receiver` otherwise.
- If no reply within 1 second, resend hello; give up after 5 attempts total.
- Use **one socket for the entire run** (relay identifies you by source address:port). Sessions idle 30s are forgotten.

### 2.3 Packet format (exact byte layout — build/parse manually, no struct memcpy)

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | `type` | 0=DATA, 1=ACK, 2=FIN |
| 1 | 1 | `reserved` | always 0 |
| 2 | 2 | `checksum` | Internet checksum (RFC 1071) |
| 4 | 4 | `seq` | counts **packets**, not bytes |
| 8 | 2 | `length` | payload length in bytes, ≤1024 |
| 10 | `length` | `payload` | file bytes (0 for ACK/FIN) |

- Header = 10 bytes; max packet = 1034 bytes.
- All multi-byte fields in **network byte order** (`htons`/`htonl`/inverses).
- **Checksum**: 16-bit one's complement of one's complement sum, computed with checksum field zeroed, odd lengths padded with one zero byte for the calc only. Verify RFC 1071 worked example: bytes `00 01 f2 03 f4 f5 f6 f7` → checksum `0x220d` (sum `0xddf2`). A valid packet's sum-with-checksum-included must equal `0xffff`.
- **Seq semantics**:
  - DATA `seq` = index of that chunk in the file (0-based); every chunk is 1024 bytes except the last.
  - FIN `seq` = number of DATA packets sent (i.e., next index after last). Empty file → single FIN with `seq=0`.
  - ACK `seq` = index of the **next packet the receiver expects** (cumulative). ACK 5 means "0–4 all arrived."
  - Files ≤16 MiB → 32-bit seq never wraps; no modular arithmetic needed.
- **Validation** — silently drop (treat as lost, no reply) unless ALL hold:
  1. datagram ≥ 10 bytes
  2. `10 + length == actual datagram size` AND `length ≤ 1024`
  3. `type ∈ {0,1,2}` AND `reserved == 0`
  4. checksum verifies
  - **Security note**: check `length` against what `recvfrom` actually returned *before* copying any payload bytes — don't trust the field blindly (buffer overflow risk, and the relay will flip bits in it).
- There is no NAK. A corrupted packet = a lost packet from the sender's perspective; the timer is the only recovery path for both.

### 2.4 Sender window diagram → rules
State: `base` (oldest unacked), `next` (next unsent), copies of all packets in `[base, next-1]`, **one** timer.
- **Send**: while `next < base + window` and data remains: send packet `next`; `next += 1`; start timer if not running.
- **On ACK** with `seq > base`: set `base = seq` (may jump multiple slots — this is what "cumulative" means); restart timer if anything still unacked, else stop it. ACK with `seq <= base` → duplicate, ignore.
- **On timeout**: resend **everything** from `base` to `next-1`; restart timer. (This is the "go back" — note it resends packets the receiver may have already seen, which is the GBN trade-off Task 6 asks you to quantify.)
- **On full window ACKed**: send FIN; treat it like one more packet (retransmit on timeout until ACKed); then exit 0.
- **Give up**: after 10 consecutive timeouts with no `base` movement → print reason, exit 2.
- Use `poll()` on the socket with the remaining timer duration as the timeout (don't busy-wait). Measure time with `clock_gettime(CLOCK_MONOTONIC, ...)` — never `time()`.
- Stop-and-wait (rdt3.0) = window of 1. If your sender is correct generically, `-w 1` and `-w 16` both work with no special-casing.

### 2.5 Receiver diagram → rules
State: single integer `expected` (next wanted index), starts at 0. For each **valid** packet:
- DATA with `seq == expected`: write payload, `expected += 1`, send `ACK expected`.
- Any other DATA (duplicate or ahead of a gap): discard, resend `ACK expected` (do **not** buffer out-of-order packets — GBN receivers are "dumb" by design).
- FIN with `seq == expected`: close file, `expected += 1`, send `ACK expected`, then **linger 2 seconds** answering repeated FINs with the same ACK, then exit 0. (This mirrors TCP TIME_WAIT — it exists because the receiver's final ACK can itself be lost.)
- If nothing valid arrives for 30 seconds: give up, exit 2.

### 2.6 Worked example sanity checks (use these as unit test fixtures)
- 2500-byte file → DATA 0 (1024B), DATA 1 (1024B), DATA 2 (452B), FIN seq=3. Clean-channel ACKs: 1,2,3,4.
- `"Hi!"` as DATA seq=2: wire bytes `00 00 96 91 00 00 00 02 00 03 48 69 21`.
- A later ACK subsumes earlier lost ACKs (no retransmission needed) — "ACK 5 covers 3 and 4 that were dropped."
- A corrupted packet and a lost packet are indistinguishable to the sender — both resolved only by timeout.

---

## 3. Required CLI (must match exactly — this is graded directly)

```
myapp send -s <session> [-w window] [-T timeout-ms] [-l loss] [-c corrupt] [-d dup] [-p port] <relay> <file>
myapp recv -s <session> [-p port] <relay> <file>
```
- `-w`: 1–64, default 8. `-T`: default 250ms. `-l`/`-c`/`-d`: default 0. `-p`: default 4250.
- Read mode from `argv[1]`; parse rest with `getopt`. Resolve `<relay>` via `getaddrinfo` (don't assume IPv4 literal).
- Exit codes: **0** success, **1** bad command line, **2** relay refusal / network failure / give-up.
- No arguments at all → print usage, exit **0** (this exact path is what `make leak` exercises).

---

## 4. Discrete Task Checklist (map to rubric §6 below as you go)

- [x] **Task 1 — CLI**: Implement argument parsing exactly as specified above; verify both modes, all flags, defaults, and exit codes.
- [ ] **Task 2 — Receiver**: Implement the `expected`-counter state machine (§2.5). Test manually with the relay on a clean channel first.
- [x] **Task 3 — Sender**: Implement the `base`/`next`/timer state machine (§2.4). Verify stop-and-wait (`-w 1`) and GBN (`-w 16`) both behave correctly. *(verified at unit level incl. in-memory e2e at w=1/w=16; live relay runs happen during Task 4 integration)*
- [ ] **Task 4 — Testable design (critical, explicitly graded)**: Split `src/lab.h` into exactly three layers:
  1. **Packets** — pure functions: encode/decode/validate/checksum. No I/O.
  2. **State machines** — sender & receiver structs + event-driven functions (`on_ack`, `on_data`, `on_timeout`, etc.) that take "now" as a parameter and return actions. **No** `sendto`/`recvfrom`/`poll`/`clock_gettime`/`fwrite` calls anywhere in this layer.
  3. **I/O** — thin glue: real socket, real clock, real file, the `poll` loop, relay handshake. Only this layer touches the OS.
- [ ] **Task 5 — Unity tests** (`make check`), including minimum required cases:
  - RFC 1071 checksum example, odd-length packet, single-bit-flip detection
  - Too-short datagram, length/size mismatch, unknown `type`
  - Receiver given a duplicate / out-of-gap packet / repeated FIN
  - Sender: full window, cumulative ACK sliding multiple slots, duplicate ACK ignored, timeout resending full window, give-up after 10 timeouts
  - Empty file; file that's an exact multiple of 1024 bytes
  - **End-to-end in-memory lossy test**: sender/receiver state machines wired through a fake channel dropping/corrupting/duplicating 20% each way, fixed-seed RNG, assert bytes-out == bytes-in. (Run with multiple seeds before trusting it.)
- [ ] **Task 6 — Measure the window**:
  1. `head -c 1048576 /dev/urandom > 1mib.bin`
  2. Run relay with `--delay 50`.
  3. For each of `(window, loss)` ∈ {(1,0), (16,0), (1,0.05), (16,0.05)} — corrupt/dup = 0, default 250ms timeout — run **3 times**, verify with `cmp`, time with `time`.
  4. Add a **Results** table to `README.md`: mean time + throughput (KiB/s) per combination.
  5. Answer in prose: (a) actual RTT implied by the window-1/no-loss run (sent 1025 packets × 1 RTT each; relay adds 100ms — explain the rest), (b) is the window-16 speedup close to 16×, why/why not, (c) why does 5% loss hurt window-16 proportionally more than window-1 (think about what a timeout forces you to resend in each case).
- [ ] **Task 7 — Coverage**: `make clean && make all && make report` → fix tests until **100% coverage**, all passing. Only system/library-call branches may be excluded.
- [ ] **Task 8 — Leak/crash check**: `make leak` then `make leak-test`. Fix every leak/crash — check retransmission buffers and error paths specifically.
- [ ] **Task 9 — README.md**: Replace entirely following the linked example template. Must include a **Design** section explaining the Task 4 three-layer split and *why* it exists, followed by the Task 6 **Results** section.
- [ ] **Task 10 — CI**: Push to GitHub, confirm CI is green, run the "Create Submission Report Via GitHub Action" workflow, download `submission-report.docx`.
- [ ] **Submit**: Upload `submission-report.docx` to Canvas; verify it arrived intact.

---

## 5. Common Pitfalls Called Out in the Spec

- Opening a new socket per packet (breaks relay's session identification) — use one socket for the whole run.
- Trusting the `length` field before checking it against the actual `recvfrom` size.
- Using `memcpy` to lay a struct directly onto the wire (padding/byte-order non-portability).
- Using wall-clock `time()` instead of `CLOCK_MONOTONIC` for timers.
- Letting the receiver buffer out-of-order packets (GBN receivers must stay "dumb" — discard and re-ACK).
- Skipping the receiver's 2-second FIN linger (causes the sender to retransmit FIN into the void and give up on a transfer that actually succeeded).
- Forgetting `--delay 50` on the relay during Task 6 timing (needed for a realistic 100ms RTT baseline).
- Picking a port already in use on shared Onyx — use `--port <n>` (20000–60000) on relay and `-p <n>` on both `myapp` invocations if 4250 is taken.

---

## 6. Rubric Mapping (100 pts total)

| # | Criterion | Pts | Satisfied by Task(s) |
|---|---|---|---|
| 1 | Packets encoded correctly; checksum correct; malformed/damaged datagrams rejected | 15 | Task 1, 4 (layer 1), 5 |
| 2 | Required CLI; relay registration w/ retries; byte-identical clean-channel transfer | 10 | Task 1, 2, 3 |
| 3 | Receiver in-order delivery, cumulative ACKs, discards/re-ACKs duplicates & out-of-order | 15 | Task 2 |
| 4 | Sender window w/ one timer, cumulative ACK sliding, full-window resend on timeout, recovers from loss/corruption/dup | 20 | Task 3 |
| 5 | Clean termination: FIN ACKed, receiver lingers, sender exits 2 after 10 fruitless timeouts | 5 | Task 2, 3 |
| 6 | Results table (4 runs) + RTT estimate + 3 analysis questions answered | 10 | Task 6 |
| 7 | Protocol logic separated from socket/clock/file I/O; Unity tests incl. seeded lossy end-to-end; 100% coverage | 15 | Task 4, 5, 7 |
| 8 | No leaks/crashes under `make leak-test`; README replaced; CI green on last push | 10 | Task 8, 9, 10 |

**Hard gate (zero override):** code must compile on **both** GitHub Codespaces and Onyx. Compiling on only one = 0 regardless of the above.

---

## 7. Suggested Execution Order

1. Environment + relay sanity check (§1).
2. Implement packet layer (encode/decode/checksum) + its unit tests first — everything else depends on it being correct (Task 4 layer 1, part of Task 5).
3. Implement sender & receiver state machines as pure, I/O-free structs (Task 4 layer 2), with unit tests driven by a fake clock/channel (Task 5 core cases + end-to-end lossy test).
4. Implement the thin I/O layer (sockets, relay handshake, poll loop, file I/O, CLI parsing) wrapping the state machines (Task 1, 4 layer 3).
5. Manual integration test against the real relay on a clean channel, then with loss/corrupt/dup individually.
6. Run Task 6 timing experiments once correctness is solid; write the Results section.
7. Drive coverage to 100% (Task 7), then fix leaks/crashes (Task 8).
8. Finalize README (Task 9), push, confirm CI, generate and submit the report (Task 10).

## Backlog (agent observations — human triages)

- Task 2 manual clean-channel relay test deferred: it needs the I/O glue (Task 4 layer 3). Packet layer, receiver, and sender state machines are all done and unit-tested (see PROGRESS.md); Task 2 stays unchecked until the manual live-relay test runs.
- Design note: ACKs arriving at the receiver are ignored (spec defines no use for them); locked in by `test_rx_ack_at_receiver_ignored`.
