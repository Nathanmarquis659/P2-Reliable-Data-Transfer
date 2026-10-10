# Project X

- Name: John Doe
- Email: johndoe@u.boisestate.edu
- Class: CS123-001

## Known Bugs or Issues

TODO: Are there any known issues?

## Experience

TODO: Describe your experience with the project (struggles, breakthroughs, etc.).

## Analysis

TODO: Provide your analysis of the results. If the assignment does not require
analysis, you can remove this section.

Here is an example of how to include a plot in your README:

![Example Image](scripts/example_plot.png)

## Results — Window Size Experiment (Task 6)

A 1 MiB random file (`head -c 1048576 /dev/urandom > 1mib.bin`) was transferred
through `cs425_relay.py --delay 50` (50 ms hold each way = 100 ms round trip)
with corrupt and duplicate rates 0 and the default 250 ms retransmission timeout.
Three runs per combination; every run finished with exit code 0 on both sides and
`cmp` confirmed the output byte-identical to the input. The timed quantity is the
wall-clock duration of the `myapp send` process (the receiver's 2 s post-transfer
linger happens after the sender has already exited and is not counted). Throughput
= 1024 KiB / mean time.

| Window | Loss  | Run 1 (s) | Run 2 (s) | Run 3 (s) | Mean (s) | Throughput (KiB/s) |
|-------:|------:|----------:|----------:|----------:|---------:|-------------------:|
| 1      | 0     |   103.546 |   103.530 |   103.537 |  103.538 |               9.9 |
| 16     | 0     |     6.610 |     6.605 |     6.596 |    6.604 |             155.1 |
| 1      | 0.05  |   133.865 |   133.339 |   132.109 |  133.104 |               7.7 |
| 16     | 0.05  |    27.079 |    24.362 |    23.205 |   24.882 |              41.2 |

### Analysis

**(a) RTT implied by the window-1, no-loss run.** With window 1 every packet
waits a full round trip before the next is sent: 1024 DATA packets plus the FIN
= 1025 packets, so 103.538 s / 1025 ≈ **101.0 ms per packet**. The relay was
configured to add exactly 100 ms (50 ms each way), so the extra ~1 ms is: the
one-time HELLO/OK handshake, which costs one extra RTT at the start and amortizes
to ~0.1 ms over 1025 packets; per-packet processing on both ends (`recvfrom`/
`sendto` syscalls, checksum computation); and OS scheduling / `poll` wakeup
granularity — all sub-millisecond contributions that add up to ~1 ms.

**(b) Is the window-16 speedup close to 16×?** Measured: 103.538 / 6.604 ≈
**15.7×** — close, but not exactly 16. On a lossless channel a window-16 sender
pipelines up to 16 packets per RTT, so the data phase takes ~1024/16 = 64 RTTs
instead of 1024. The shortfall from 16× is fixed overhead that does not shrink
with window size: the HELLO/OK handshake at the start and the FIN/ACK exchange at
the end each cost a full RTT regardless of window. With only ~65 RTTs in total,
those constants dilute the ratio (ideal ≈ 1026/66.5 ≈ 15.4), with small OS jitter
on top.

**(c) Why does 5% loss hurt window-16 proportionally more than window-1?** Loss
slows window 1 by 29% (103.5 → 133.1 s) but window 16 by 277% (6.6 → 24.9 s).
This is the Go-Back-N trade-off. With window 1, a lost packet — or a lost ACK —
costs one timeout and a retransmission of exactly one packet; recovery is ~1 RTT
and nothing that already arrived is resent. With window 16, the same single loss
triggers a 250 ms timeout and a full retransmission of up to 16 packets, most of
which the receiver already has and will discard. With 16 datagrams in flight, the
chance that at least one of them (or its ACK) is lost in any given window cycle
is ≈ 1 − 0.95^16 ≈ 56%, so nearly every window cycle pays a timeout plus a
retransmission burst. The relay's own counters confirm it: the w=16/loss-0.05
runs forwarded ~2000 datagrams per direction for a file of only 1025 — roughly
double, from whole-window resends.
