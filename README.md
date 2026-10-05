# ft_ping

**A from-scratch reimplementation of `ping` in C**

Raw ICMP sockets | IPv4 | inetutils-compatible behavior

> [!NOTE]
> This project is intended to match the observable behavior of
> **inetutils-2.0**, not to call or reuse the system `ping` implementation.

## Contents

- [Overview](#overview)
- [Quick start](#quick-start)
- [Usage](#usage)
- [Notable behavior](#notable-behavior)
- [Validation](#validation)

## Overview

`ft_ping` is a from-scratch implementation of `ping`, built against a raw ICMP
socket. Behavioral fidelity is measured against **inetutils-2.0** (`ping -V`),
as required by the subject.

No system `ping` binary or its source is used anywhere in this implementation.

## Quick start

Build the project from its root:

```sh
make        # Build libft, then ft_ping
make re     # Full rebuild
make clean  # Remove object files
make fclean # Remove object files and the binary
```

This produces an `ft_ping` executable in the project root. Because it opens a
raw ICMP socket, it requires `CAP_NET_RAW` and is normally run with `sudo`.

## Usage

```sh
sudo ./ft_ping [OPTIONS] HOST
```

`HOST` may be an IPv4 literal, a hostname, or an FQDN. Reverse DNS resolution
of a reply's source address is never performed, by design.

### Options

#### Mandatory

| Option | Description |
|:--|:--|
| `-v`, `--verbose` | Report non-echo-reply ICMP messages, such as destination unreachable and time exceeded, instead of discarding them silently. The program continues running. |
| `-?`, `--help` | Print usage information and exit. |

#### Bonus

| Option | Description |
|:--|:--|
| `-f`, `--flood` | Send as fast as replies come back, capped at 100 packets per second. Prints `.` per request and `\b` per reply instead of the normal per-reply line. |
| `-l`, `--preload=NUMBER` | Send `NUMBER` packets back-to-back before entering normal mode. |
| `-n`, `--numeric` | Accepted for compatibility. It has no visible effect because this implementation never performs reverse-DNS lookups. |
| `-p`, `--pattern=PATTERN` | Fill the ICMP payload with the given hex byte pattern instead of the default incrementing fill. |
| `-r`, `--ignore-routing` | Bypass the normal routing tables (`SO_DONTROUTE`). |
| `-s`, `--size=NUMBER` | Set the number of data octets to send (default `56`). |
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
| `--ip-timestamp` | Option is sent, but returned timestamp values are not displayed |

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
receive window and appear as packet loss even when the target is healthy.

This implementation checks the deadline before each regular send and limits the
reply wait to the remaining time. A reply that arrives after the overall
deadline is still counted as lost, as expected for a bounded run.

### IP timestamp output

`--ip-timestamp` causes the kernel to attach the option to outgoing requests
(confirmed on the wire via `tcpdump`). Intermediate hops and the destination
fill the reserved slots as RFC 791 describes, but this implementation does not
parse and print the returned values as canonical `ping -v` does.

## Validation

The project was validated against a real inetutils build using:

- Equivalence checks with flag-by-flag output comparisons.
- Stress tests covering large `-l` bursts, sustained `-f` runs, concurrent
  instances, and `SIGINT` handling.
- Targeted edge cases, including argument boundaries and ICMP sequence-number
  wraparound at 65536.

No crashes, hangs, or resource leaks were found.

---

<div align="center">

Made for low-level networking practice and behavioral compatibility.

</div>
