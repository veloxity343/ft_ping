# ft_ping

**A from-scratch reimplementation of `ping` in C**

Raw ICMP sockets | IPv4 | inetutils-compatible behavior

> [!NOTE]
> This project is intended to match the observable behavior of
> **inetutils-2.0**, not to call or reuse the system `ping` implementation.

## Contents

- [Overview](#overview)
- [Requirements](#requirements)
- [Quick start](#quick-start)
- [Usage](#usage)
- [Options](#options)
- [Notable behavior](#notable-behavior)
- [Differences from inetutils](#differences-from-inetutils)
- [Exit codes](#exit-codes)
- [Project layout](#project-layout)
- [Validation](#validation)

## Overview

`ft_ping` is a from-scratch implementation of `ping`, built against a raw ICMP
socket. Behavioral fidelity is measured against **inetutils-2.0** (`ping -V`),
as required by the subject.

No system `ping` binary or its source is used anywhere in this implementation.

## Requirements

- Linux (a kernel newer than 3.14, as the subject requires)
- `gcc` and `make`
- Permission to open a raw ICMP socket (`CAP_NET_RAW`), normally by running as
  root. Without it the program stops immediately with
  `ft_ping: socket: Operation not permitted (are you root?)`.

To avoid `sudo`, the capability can be granted to the binary instead, which is
the standard approach for raw-socket tools (not exercised by this project's own
tests):

```sh
sudo setcap cap_net_raw+ep ./ft_ping
```

## Quick start

Build the project from its root:

```sh
make        # Build libft, then ft_ping
make re     # Full rebuild
make clean  # Remove object files
make fclean # Remove object files and the binary
```

This produces an `ft_ping` executable in the project root. `make` recompiles
only the files that changed.

## Usage

```sh
sudo ./ft_ping [OPTIONS] HOST
```

`HOST` may be an IPv4 literal, a hostname, or an FQDN. Reverse DNS resolution
of a reply's source address is never performed, by design.

### Example output

A normal run (`-w` stops the run after that many seconds):

```text
$ sudo ./ft_ping -w 1 127.0.0.1
PING 127.0.0.1 (127.0.0.1): 56 data bytes
64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.037 ms
--- 127.0.0.1 ping statistics ---
1 packets transmitted, 1 packets received, 0% packet loss
round-trip min/avg/max/stddev = 0.037/0.037/0.037/0.000 ms
```

`-v` with a TTL too small to reach the target reports the ICMP error and keeps
running (the first router answers `Time to live exceeded`). Under `-v` the
quoted original request is shown too: a hex dump of its IP header, a table of
its fields, and what it carried:

```text
$ sudo ./ft_ping --ttl 1 -v -w 1 10.77.2.5
PING 10.77.2.5 (10.77.2.5): 56 data bytes, id 0xc153 = 49491
92 bytes from 10.77.1.2: Time to live exceeded
IP Hdr Dump:
 4500 0054 930d 4000 0101 cefc 0a4d 0101 0a4d 0205 
Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data
 4  5  00 0054 930d   2 0000  01  01 cefc 10.77.1.1  10.77.2.5 
ICMP: type 8, code 0, size 64, id 0xc153, seq 0x0001
--- 10.77.2.5 ping statistics ---
1 packets transmitted, 0 packets received, 100% packet loss
```

Without `-v`, an error about a request sent to the target still prints its
first line (`92 bytes from 10.77.1.2: Time to live exceeded`), as in
inetutils; `-v` adds the detail above and also reports errors about requests
sent to any other destination.

`--ip-timestamp` prints the timestamps recorded in the reply, as raw
milliseconds since midnight UT:

```text
$ sudo ./ft_ping --ip-timestamp=tsonly -w 1 127.0.0.1
PING 127.0.0.1 (127.0.0.1): 56 data bytes
64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.033 ms
TS:	50912722 ms
	50912722 ms
	50912722 ms
	50912722 ms

--- 127.0.0.1 ping statistics ---
1 packets transmitted, 1 packets received, 0% packet loss
round-trip min/avg/max/stddev = 0.033/0.033/0.033/0.000 ms
```

`-f` prints a dot per request and a backspace per reply, so the output is only
readable through `cat -v` (shown elided):

```text
$ sudo ./ft_ping -f -w 1 127.0.0.1 | cat -v
PING 127.0.0.1 (127.0.0.1): 56 data bytes
^H.^H.^H.^H. ... ^H.--- 127.0.0.1 ping statistics ---
100 packets transmitted, 100 packets received, 0% packet loss
```

## Options

### Mandatory

| Option | Description |
|:--|:--|
| `-v`, `--verbose` | Report non-echo-reply ICMP messages, such as destination unreachable and time exceeded, with the quoted original request, instead of only the one-line form. The program continues running. |
| `-?`, `--help` | Print usage information and exit. |

### Bonus

| Option | Description |
|:--|:--|
| `-f`, `--flood` | Send as fast as replies come back, capped at 100 packets per second. Prints `.` per request and `\b` per reply instead of the normal per-reply line. |
| `-l`, `--preload=NUMBER` | Send `NUMBER` packets back-to-back before entering normal mode. |
| `-n`, `--numeric` | Do not reverse-resolve the addresses recorded by `--ip-timestamp=tsaddr`: they print as plain IPs. Without `-n` they are resolved to a hostname when possible. Reply lines never show a resolved name either way. |
| `-p`, `--pattern=PATTERN` | Fill the ICMP payload with the given hex byte pattern instead of the default incrementing fill. |
| `-r`, `--ignore-routing` | Bypass the normal routing tables (`SO_DONTROUTE`). Only directly attached hosts can be reached; any other target sends nothing. |
| `-s`, `--size=NUMBER` | Set the number of data octets to send (default `56`, maximum `65399`, as in canonical). Below 16 there is no room for the embedded send timestamp, so replies print without `time=` and the `round-trip` line is omitted, as in canonical. |
| `-T`, `--tos=NUM` | Set the IP type-of-service field. |
| `--ttl=N` | Set the IP time-to-live field (default `64`). |
| `-w`, `--timeout=N` | Stop after `N` seconds, regardless of how many packets have been sent or received. |
| `-W`, `--linger=N` | Wait `N` seconds for a reply before considering it lost (default `1`). |
| `--ip-timestamp=FLAG` | Attach an RFC 791 IP Timestamp option to outgoing packets. `FLAG` is `tsonly` (timestamps only) or `tsaddr` (address and timestamp pairs). |

<details>
<summary>Option quick reference</summary>

| Short form | Long form |
|:--|:--|
| `-v` | `--verbose` |
| `-?` | `--help` |
| `-f` | `--flood` |
| `-l` | `--preload=NUMBER` |
| `-n` | `--numeric` |
| `-p` | `--pattern=PATTERN` |
| `-r` | `--ignore-routing` |
| `-s` | `--size=NUMBER` |
| `-T` | `--tos=NUM` |
| - | `--ttl=N` |
| `-w` | `--timeout=N` |
| `-W` | `--linger=N` |
| - | `--ip-timestamp=FLAG` |

</details>

## Notable behavior

### Reply addresses

Reply lines always show the plain IP address. The implementation does not
resolve a reply's source address back to a DNS name, including when the target
was provided as an FQDN.

| Behavior | Result |
|:--|:--|
| FQDN target | Reply lines still show the resolved IPv4 address |
| Duplicate reply | Marked `(DUP!)` and counted separately |
| `--ip-timestamp` | Option is sent, and the timestamps recorded in the reply are parsed and printed as a `TS:` block |

### Duplicate detection

Replies are tracked in a 65536-bit bitmap, with one bit per possible ICMP
sequence number. This remains correct when multiple requests are outstanding
(`-l`, `-f`) or replies arrive out of order. The bitmap is cleared whenever the
sequence counter wraps, preventing false positives during long runs.

A true duplicate is marked `(DUP!)` and counted separately from unique replies
in the summary line.

### Deadline handling

Deadline handling differs slightly from canonical `ping` by design. Canonical
`ping` checks its `-w` deadline immediately after sending a packet, before
waiting for that packet's reply. Its final packet may therefore receive no
receive window and appear as packet loss even when the target is healthy. For
example, a plain `-w 1` run against loopback reports `2 transmitted, 1 received`
with canonical `ping`, and `1 transmitted, 1 received` here.

This implementation checks the deadline before each regular send and limits the
reply wait to the remaining time. A reply that arrives after the overall
deadline is still counted as lost, as expected for a bounded run.

### IP timestamp output

`--ip-timestamp` causes the kernel to attach the option to outgoing requests
(confirmed on the wire via `tcpdump`). Intermediate hops and the destination
fill the reserved slots as RFC 791 describes. The option is then read back out
of each reply and printed after the reply line in the same layout as canonical
`ping`: a `TS:` label, one tab-indented `<n> ms` value per recorded slot (with
the address first under `tsaddr`), an overflow line if any hop ran out of
slots, and a trailing blank line. The values are raw milliseconds, not a clock
time. Nothing is printed under `-f`, which replaces the whole reply line.

### Preload

`-l` sends its burst without pacing and then collects that many replies. When
no `-w` is given, that collection phase is limited to one `-W` window in total,
so a lossy or unreachable target cannot stall startup for `preload * linger`
seconds.

## Differences from inetutils

The subject requires matching inetutils-2.0 for the reply, banner, statistics
and `-v` output, and that part is compared line by line (see
[Validation](#validation)). The items below are the known differences, each
either deliberate or outside what the subject asks for.

| Area | inetutils | ft_ping |
|:--|:--|:--|
| First `icmp_seq` | `0` | `1` |
| `-w` deadline | Checked right after a send, so the last packet can be sent with no time left to answer it | Checked before each send, so every packet sent gets a reply window |
| `-c COUNT`, `-i INTERVAL`, `-q` | Supported | Not implemented (use `-w` to bound a run) |
| `-R`, `-V`, `--usage`, `--echo`, `--type` | Supported | Not implemented |
| Record route | `-R` records and prints the route | Not implemented (`-r` is `--ignore-routing`, a different option) |
| Address family | IPv4 and IPv6 (`ping6`) | IPv4 only |
| `-?` text | Full help with every option | Lists only the options implemented here |
| Error messages | Written to stderr, e.g. ``ping: invalid value (`abc' near `abc')`` | Written to stdout as `ft_ping: invalid argument 'abc' for '--size'`, then a usage hint |
| `-p` pattern | Longer patterns accepted | At most 16 bytes (32 hex digits) |
| Reply source | May be reverse-resolved to a name | Always the plain IP, as the subject requires |

## Exit codes

| Code | Meaning |
|:--|:--|
| `0` | Normal run, or `-?` |
| `1` | Invalid option or argument, missing host, or a socket or `setsockopt` failure (including missing privileges) |
| `2` | The host name could not be resolved |

Canonical differs here: it exits `1` when no reply was received or the host is
unknown, and `64` for a usage error. This implementation exits `0` after any
normal run, whether or not replies arrived.

## Project layout

| File | Responsibility |
|:--|:--|
| `src/main.c` | Entry point, send/receive loop, pacing for the regular, flood and preload modes |
| `src/parsing.c` | Option parsing with `getopt_long`, argument validation, `-?` help text |
| `src/resolve.c` | Target resolution (IPv4 literal, hostname, FQDN) and the reverse lookup used by `-n` |
| `src/socket.c` | Raw socket creation and per-option socket settings (TTL, TOS, `SO_DONTROUTE`, IP options) |
| `src/icmp.c` | Packet construction and checksum, send and receive, reply matching, duplicate tracking, IP timestamp parsing |
| `src/output.c` | Every line the program prints: banner, replies, `TS:` block, statistics, ICMP errors |
| `src/utils.c` | `SIGINT` handler and time arithmetic |
| `inc/ft_ping.h` | Shared types, constants and prototypes |

## Validation

The reference is **inetutils-2.0**, but the version available for live
comparison was **2.6**. The 2.0 and 2.6 sources were compared directly:
the send/receive loop, preload and flood logic and the IP timestamp option
construction are identical, apart from `timeval` becoming `timespec` and
`select` becoming `pselect`. Results from 2.6 therefore carry over.

Test suite:

- **Equivalence:** the verbose banner is byte-identical to inetutils once the
  PID-derived id is masked, and the `--ip-timestamp=tsonly` `TS:` block has the
  same structure. Beyond that, the default run, `-p`, `-l`, `--ttl`/`-T`,
  `--ip-timestamp=tsaddr` (with and without `-n`), `-s` from 0 to 65399 and
  the ICMP error output (one-line and `-v` forms, for TTL expiry and no
  route, on a private client/router network) are each diffed against the real binary with only
  seq/ttl/time and packet counts masked. Bad option values are rejected by
  both programs with the same exit status.
- **Wire level:** with `tcpdump -vv`, a plain request is 84 bytes with no IP
  options, and with `--ip-timestamp` it is 124 bytes carrying a decoded `TS`
  option on both the request and the reply.
- **Real network:** replies from a public IP and from an FQDN, and a genuine
  `Time to live exceeded` from the first router under `-v --ttl 1`, with the
  program continuing to a normal summary afterwards.
- **Stress:** a 69,412-packet flood (past the 65536 sequence wraparound) ended
  with 0% loss and no false duplicates; 10 rapid mixed-flag runs with no
  crashes; `SIGINT` during a flood and during a live external ping both exit
  cleanly and still print the summary; two concurrent instances do not see
  each other's replies; file descriptors and memory stayed constant over an
  8 second flood.
- **Arguments:** out-of-range and malformed values are rejected with the exact
  message `ft_ping: invalid argument 'X' for '--option'`, before any socket is
  opened.

Test commands:

```sh
./ft_ping -?                                              # exact help text, exit 0
sudo ./ft_ping -w 3 google.com                            # FQDN banner shows hostname + IP
sudo ./ft_ping --ttl 1 -v -w 3 8.8.8.8                    # ICMP error, program keeps running
sudo ./ft_ping -l 5 -w 2 127.0.0.1                        # 7 transmitted, 7 received
sudo ./ft_ping -f -w 1 127.0.0.1 | cat -v                 # dot/backspace pairs
sudo ./ft_ping --ip-timestamp=tsaddr -n -w 1 127.0.0.1    # numeric address in TS: block
sudo ./ft_ping --ip-timestamp=tsaddr -w 1 127.0.0.1       # resolved to "localhost"
./ft_ping -s 65400 127.0.0.1                              # rejected: over the 65399 maximum
```
