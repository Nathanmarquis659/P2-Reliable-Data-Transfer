# P2 - Reliable Data Transfer

- Name: Nathan Marquis
- Email: nathanmarquis659@u.boisestate.edu
- Class: CS425-001

## Experience

This project was actually quite difficult for me, the extend of code was pretty heavy and confusing to set up, but with the help of some AI tools it helped a lot. I had it work through it with me and generate some great explanations of what was going on, and especially why we had the three layer split involved at all. I think this assignment would normally be too much work for a class like this, but due to our ability to use agentic tools it was pretty reasonable of an ask and did in fact give a lot of insight into these protocols and how they function. 

## Design
This program had a three layer split (packets, state machines, I/O) which was meant to make our protocol logic be separate and testable, independent of the OS until required. The packets served as pure byte level function meant to handle the simple packet data, whether to encode, checksum, or validate. The second layer, state machines, were meant to fully run through the logical side in a testable way, which means no OS integration such as sockets, clocks or files. The final layer, I/O, is meant to finalize the actual interface to the OS and provide sockets and access to complete the entire cycle of each protocol. 


## Results

| Window | Loss  | Run 1 (s) | Run 2 (s) | Run 3 (s) | Mean (s) | Throughput (KiB/s) |
|-------:|------:|----------:|----------:|----------:|---------:|-------------------:|
| 1      | 0     |   103.546 |   103.530 |   103.537 |  103.538 |               9.9 |
| 16     | 0     |     6.610 |     6.605 |     6.596 |    6.604 |             155.1 |
| 1      | 0.05  |   133.865 |   133.339 |   132.109 |  133.104 |               7.7 |
| 16     | 0.05  |    27.079 |    24.362 |    23.205 |   24.882 |              41.2 |

**(a) Round-Trip Time.** With window 1 every packet
waits a full round trip before the next is sent so the 1024 DATA packets plus the FIN
equal a total of 1025 packets. 103.538 s / 1025 = 101.0 ms per packet. The relay was
configured to add exactly 100 ms , so the extra 1 ms is the
one-time HELLO/OK handshake, which costs one extra round trip at the start. Packet processing on both ends and OS work are all tiny contributions that also add up to the additional time.

**(b) Window Speedup Time.** Measured as 103.538 / 6.604 =
15.7, close, but not exactly the 16 factor. On a lossless channel a window-16 sender
pipelines up to 16 packets per, so the data phase takes around 1024/16 = 64 round trips
instead of 1024. The shortfall from 16× is fixed overhead that does not shrink
with window size like the HELLO/OK handshake at the start and the FIN/ACK exchange at
the end each cost a full round trip regardless of window. With only 65 trips in total,
those constants give some efficiency reduction to the ratio with small OS jitter
on top.

**(c) Proportional Loss.** Loss
slows window 1 by 29% (103.5 → 133.1 s) but window 16 by 277% (6.6 → 24.9 s).
This is the Go-Back-N trade-off in action. With window 1, a lost packet/ACK
costs one timeout and a retransmission of exactly one packet so the recovery is only 1 round trip
and nothing that already arrived is resent. With window 16, the same single loss
triggers a 250 ms timeout and a full retransmission of up to 16 packets, most of
which the receiver already has and will throw out.
