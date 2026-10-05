#include "ft_ping.h"

/**
 * @brief Prints the "PING host (ip): N data bytes" banner, once before
 * the send/receive loop starts.
 *
 * The byte count shown is the payload size (packet_size minus the
 * 8-byte ICMP header), matching what canonical labels "data bytes".
 * Under -v/--verbose, also shows our own ICMP identifier (PID,
 * truncated to 16 bits) - matching canonical's verbose banner exactly.
 */
void	print_start_banner(void)
{
	ft_printf("PING %s (%s): %d data bytes", g_ping.hostname,
		g_ping.ip_str, g_ping.opts.packet_size - (int)sizeof(struct icmphdr));
	if (g_ping.opts.verbose)
		ft_printf(", id 0x%04x = %u", (unsigned short)g_ping.pid,
			(unsigned short)g_ping.pid);
	ft_printf("\n");
}

/**
 * @brief Prints one reply's line: "N bytes from ip: icmp_seq=S ttl=T
 * time=R ms", with " (DUP!)" appended if this is a repeat of an
 * already-counted reply.
 *
 * Under -f/--flood this collapses to a single backspace instead,
 * erasing one of the dots printed at send time (main.c) - dup or not,
 * canonical doesn't distinguish them in flood output either.
 *
 * @param bytes ICMP payload size of the reply (excludes the IP header).
 * @param seq   The reply's sequence number.
 * @param ttl   TTL of the reply's IP header, as it arrived here.
 * @param rtt   Round-trip time in milliseconds, or a negative value when
 *              the reply was not timed (-s below 16: no room for the
 *              embedded timestamp), in which case the " time=" field is
 *              left out, as canonical does.
 * @param dup   1 if this reply is a duplicate (see seq_seen(), icmp.c),
 *              else 0.
 */
void	print_reply(int bytes, int seq, int ttl, double rtt, int dup)
{
	if (g_ping.opts.flood)
	{
		ft_printf("\b");
		return ;
	}
	ft_printf("%d bytes from %s: icmp_seq=%d ttl=%d",
		bytes, g_ping.ip_str, seq, ttl);
	if (rtt >= 0.0)
		ft_printf(" time=%.3f ms", rtt);
	if (dup)
		ft_printf(" (DUP!)");
	ft_printf("\n");
}

/**
 * @brief Computes packet loss as a whole-number percentage, truncated
 * (matching canonical's integer-division formula, not rounded).
 *
 * Based on unique replies only - stats.received never includes
 * duplicates (see receive_ping(), icmp.c), so this can't under-report
 * loss just because some replies were counted twice.
 *
 * @return 0-100. 0 if nothing was transmitted yet.
 */
static int	packet_loss_pct(void)
{
	if (g_ping.stats.transmitted == 0)
		return (0);
	return (((g_ping.stats.transmitted - g_ping.stats.received) * 100)
		/ g_ping.stats.transmitted);
}

/**
 * @brief Prints the closing "round-trip min/avg/max/stddev" line.
 *
 * stddev is the standard deviation of observed RTTs, computed from the
 * running sum and sum-of-squares (avoids keeping every individual
 * sample around) via sqrt(sum2/n - avg^2). Every matched reply is timed
 * into those sums, duplicates included (see update_rtt_stats(), icmp.c),
 * so n is received + duplicates, not received alone - dividing by
 * received only would skew the average and could drive the variance
 * negative. Only called when at least one reply was received (see
 * print_statistics()), so n is never 0. The variance is clamped at 0:
 * floating-point rounding can leave it a hair below zero when every RTT
 * is (nearly) identical, and sqrt() of that would print garbage.
 *
 * The subject explicitly exempts this line from needing to match
 * canonical's exact formatting.
 */
static void	print_rtt_line(void)
{
	double	n;
	double	avg;
	double	var;
	double	stddev;

	n = g_ping.stats.received + g_ping.stats.duplicates;
	avg = g_ping.stats.rtt_sum / n;
	var = g_ping.stats.rtt_sum2 / n - avg * avg;
	if (var < 0.0)
		var = 0.0;
	stddev = sqrt(var);
	ft_printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
		g_ping.stats.rtt_min, avg, g_ping.stats.rtt_max, stddev);
}

/**
 * @brief Prints the final summary block, once the send/receive loop
 * stops (deadline reached or SIGINT).
 *
 * Format matches canonical field-for-field: "N transmitted, M received,"
 * then an optional "+K duplicates," if any occurred, then either a
 * packet-loss percentage or (if received somehow exceeds transmitted -
 * a sanity trip-wire, not expected in normal operation) canonical's
 * "somebody is printing forged packets!" message, then the round-trip
 * line if at least one reply was received and timed (no round-trip line
 * for -s below 16, where nothing is timed - as in canonical).
 */
void	print_statistics(void)
{
	ft_printf("--- %s ping statistics ---\n", g_ping.hostname);
	ft_printf("%d packets transmitted, %d packets received, ",
		g_ping.stats.transmitted, g_ping.stats.received);
	if (g_ping.stats.duplicates > 0)
		ft_printf("+%d duplicates, ", g_ping.stats.duplicates);
	if (g_ping.stats.transmitted > 0)
	{
		if (g_ping.stats.received > g_ping.stats.transmitted)
			ft_printf("-- somebody is printing forged packets!");
		else
			ft_printf("%d%% packet loss", packet_loss_pct());
	}
	ft_printf("\n");
	if (g_ping.stats.received > 0 && TIMING(g_ping.opts.packet_size))
		print_rtt_line();
}

/**
 * One ICMP (type, code) pair and the text canonical prints for it.
 * Type/code are the plain RFC 792 numbers (3 = destination unreachable,
 * 5 = redirect, 11 = time exceeded) rather than the platform's ICMP_*
 * macros, which are spelled differently on macOS.
 */
typedef struct s_icmp_code
{
	int			type;
	int			code;
	const char	*text;
}	t_icmp_code;

static const t_icmp_code	g_icmp_codes[] = {
	{3, 0, "Destination Net Unreachable"},
	{3, 1, "Destination Host Unreachable"},
	{3, 2, "Destination Protocol Unreachable"},
	{3, 3, "Destination Port Unreachable"},
	{3, 4, "Fragmentation needed and DF set"},
	{3, 5, "Source Route Failed"},
	{3, 6, "Network Unknown"},
	{3, 7, "Host Unknown"},
	{3, 8, "Host Isolated"},
	{3, 11, "Destination Network Unreachable At This TOS"},
	{3, 12, "Destination Host Unreachable At This TOS"},
	{3, 13, "Packet Filtered"},
	{3, 14, "Precedence Violation"},
	{3, 15, "Precedence Cutoff"},
	{5, 0, "Redirect Network"},
	{5, 1, "Redirect Host"},
	{5, 2, "Redirect Type of Service and Network"},
	{5, 3, "Redirect Type of Service and Host"},
	{11, 0, "Time to live exceeded"},
	{11, 1, "Frag reassembly time exceeded"},
	{0, 0, NULL}
};

/**
 * @brief Looks up the description for an ICMP (type, code) pair.
 *
 * @param type An ICMP type (3, 5 or 11 - the ones with a code table).
 * @param code The type-specific code value.
 * @return A static description string, or NULL if this exact
 *         type/code combination isn't named in g_icmp_codes.
 */
static const char	*icmp_code_text(int type, int code)
{
	int	i;

	i = 0;
	while (g_icmp_codes[i].text)
	{
		if (g_icmp_codes[i].type == type && g_icmp_codes[i].code == code)
			return (g_icmp_codes[i].text);
		i++;
	}
	return (NULL);
}

/**
 * @brief Prints the quoted original IP header of an ICMP error as
 * canonical does: under -v a hex dump of its 20 fixed bytes, then (always)
 * a labelled one-row table of its fields, with any IP options appended
 * as hex.
 *
 * @param orig The embedded original datagram, starting at its IP header.
 *             The caller guarantees the whole header (and 8 bytes past
 *             it) is present.
 */
static void	print_orig_ip_header(const unsigned char *orig)
{
	unsigned short	len;
	unsigned short	id;
	unsigned short	off;
	struct in_addr	addr;
	int				i;
	int				hlen;

	hlen = (orig[0] & 0x0f) * 4;
	if (g_ping.opts.verbose)
	{
		ft_printf("IP Hdr Dump:\n ");
		i = 0;
		while (i < (int)sizeof(struct ip))
		{
			ft_printf("%02x%s", orig[i], (i % 2) ? " " : "");
			i++;
		}
		ft_printf("\n");
	}
	ft_printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	ft_memcpy(&len, orig + 2, 2);
	if (len > 0x2000)
		len = ntohs(len);
	id = (unsigned short)((orig[4] << 8) | orig[5]);
	off = (unsigned short)((orig[6] << 8) | orig[7]);
	ft_printf(" %1x  %1x  %02x %04x %04x", orig[0] >> 4, orig[0] & 0x0f,
		orig[1], len, id);
	ft_printf("   %1x %04x  %02x  %02x %04x", (off & 0xe000) >> 13,
		off & 0x1fff, orig[8], orig[9], (orig[10] << 8) | orig[11]);
	ft_memcpy(&addr, orig + 12, 4);
	ft_printf(" %s ", inet_ntoa(addr));
	ft_memcpy(&addr, orig + 16, 4);
	ft_printf(" %s ", inet_ntoa(addr));
	i = (int)sizeof(struct ip);
	while (i < hlen)
		ft_printf("%02x", orig[i++]);
	ft_printf("\n");
}

/**
 * @brief Prints the quoted original datagram of an ICMP error: its IP
 * header (see print_orig_ip_header()) followed, for TCP, UDP and ICMP, by
 * one line naming the ports or the ICMP type/code (and, for an echo, its
 * id and sequence number).
 *
 * @param orig The embedded original datagram, starting at its IP header.
 */
static void	print_orig_datagram(const unsigned char *orig)
{
	const unsigned char	*cp;
	int					hlen;
	int					total;

	print_orig_ip_header(orig);
	hlen = (orig[0] & 0x0f) * 4;
	total = (orig[2] << 8) | orig[3];
	cp = orig + hlen;
	if (orig[9] == IPPROTO_TCP || orig[9] == IPPROTO_UDP)
		ft_printf("%s: from port %u, to port %u (decimal)\n",
			(orig[9] == IPPROTO_TCP) ? "TCP" : "UDP",
			cp[0] * 256 + cp[1], cp[2] * 256 + cp[3]);
	else if (orig[9] == IPPROTO_ICMP)
	{
		ft_printf("ICMP: type %u, code %u, size %u", cp[0], cp[1],
			total - hlen);
		if (cp[0] == ICMP_ECHOREPLY || cp[0] == ICMP_ECHO)
			ft_printf(", id 0x%04x, seq 0x%04x", cp[4] * 256 + cp[5],
				cp[6] * 256 + cp[7]);
		ft_printf("\n");
	}
}

/**
 * @brief Prints one non-echo-reply ICMP error message in canonical's
 * format: "N bytes from <addr>: <description>", followed - under -v - by
 * the quoted original datagram.
 *
 * Destination unreachable, redirect and time exceeded are described by
 * (type, code) via g_icmp_codes ("<kind>, Unknown Code: N" if the code
 * isn't listed). Source quench and parameter problem have fixed texts
 * and always show the quoted datagram, as canonical does.
 *
 * @param from_ip  Numeric source address of the error message (never
 *                 reverse-resolved - see the mandatory no-reverse-DNS
 *                 requirement).
 * @param icmp_len Size of the ICMP message (outer IP header excluded).
 * @param icmp_hdr The error's ICMP header.
 * @param orig     The embedded original datagram, starting at its IP
 *                 header (validated by the caller, icmp.c).
 */
void	print_icmp_error(const char *from_ip, int icmp_len,
		const struct icmphdr *icmp_hdr, const unsigned char *orig)
{
	const char		*text;
	struct in_addr	gw;

	ft_printf("%d bytes from %s: ", icmp_len, from_ip);
	if (icmp_hdr->type == ICMP_SOURCE_QUENCH)
		ft_printf("Source Quench\n");
	else if (icmp_hdr->type == ICMP_PARAMETERPROB)
	{
		gw.s_addr = icmp_hdr->un.gateway;
		ft_printf("Parameter problem: IP address = %s\n", inet_ntoa(gw));
	}
	else
	{
		text = icmp_code_text(icmp_hdr->type, icmp_hdr->code);
		if (text)
			ft_printf("%s\n", text);
		else
			ft_printf("%s, Unknown Code: %d\n",
				(icmp_hdr->type == ICMP_DEST_UNREACH) ? "Dest Unreachable"
				: (icmp_hdr->type == ICMP_REDIRECT) ? "Redirect"
				: "Time exceeded", icmp_hdr->code);
	}
	if (icmp_hdr->type == ICMP_SOURCE_QUENCH
		|| icmp_hdr->type == ICMP_PARAMETERPROB || g_ping.opts.verbose)
		print_orig_datagram(orig);
}

/**
 * @brief Formats one recorded timestamp exactly as canonical's
 * ping_cvt_time() does: bits 0-30 are the millisecond value, bit 31
 * marks a non-standard clock source (angle brackets around the value
 * signal that, per RFC 791) rather than being part of the count.
 *
 * Confirmed against inetutils 2.6 source (ping_common.c) - the name
 * suggests a clock-format conversion, but it does not convert to
 * HH:MM:SS; it just prints the raw millisecond count.
 *
 * @param raw The recorded value exactly as read off the wire (host byte
 *            order, un-masked - t_ts_option.times entries).
 */
static void	print_ts_value(unsigned int raw)
{
	unsigned int	ms;

	ms = raw & 0x7fffffff;
	if (raw & 0x80000000)
		ft_printf("<%u> ms", ms);
	else
		ft_printf("%u ms", ms);
}

/**
 * @brief Prints the address for one tsaddr slot: the resolved hostname
 * followed by the numeric address in parentheses if -n allowed a
 * reverse lookup and it succeeded, otherwise just the numeric address.
 * No trailing separator - the timestamp printed right after supplies
 * its own leading tab.
 *
 * @param raw_addr Recorded address, network byte order (straight from
 *                 t_ts_option.addrs - do not host-order convert).
 */
static void	print_ts_addr(unsigned int raw_addr)
{
	struct in_addr	addr;
	char			host[256];

	addr.s_addr = raw_addr;
	ft_printf("\t");
	if (resolve_reverse(addr, host, sizeof(host)))
		ft_printf("%s (%s)", host, inet_ntoa(addr));
	else
		ft_printf("%s", inet_ntoa(addr));
}

/**
 * @brief Prints the IP Timestamp option recorded in a reply, if the
 * reply carried one - byte-for-byte matching canonical's print_ip_opt()
 * for the IPOPT_TS case (verified against inetutils 2.6 source and a
 * live capture).
 *
 * print_reply() has already terminated the main reply line with its own
 * newline, so unlike canonical (which keeps that line open and supplies
 * the separating newline itself, via "\nTS:") this starts directly with
 * "TS:". One line per filled slot: for tsaddr, the recording host's
 * address (numeric, or resolved per -n/--numeric - see
 * resolve_reverse()) immediately followed by its timestamp; for tsonly,
 * just the timestamp. A final blank line is always printed when the
 * option was present, matching canonical's own unconditional trailing
 * newline after the option dump. No-op if the reply carried no
 * timestamp option (ts->present == 0) or under -f/--flood, which
 * already replaces the whole per-reply line with a single backspace.
 *
 * @param ts Option data already decoded by parse_ip_timestamp_reply().
 */
void	print_ip_timestamp_option(const t_ts_option *ts)
{
	int	i;

	if (!ts->present || g_ping.opts.flood)
		return ;
	ft_printf("TS:");
	i = 0;
	while (i < ts->count)
	{
		if (ts->tsaddr)
			print_ts_addr(ts->addrs[i]);
		ft_printf("\t");
		print_ts_value(ts->times[i]);
		ft_printf("\n");
		i++;
	}
	if (ts->overflow > 0)
		ft_printf("\t(%d overflowing hosts)", ts->overflow);
	ft_printf("\n");
}
