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
 * @param rtt   Round-trip time in milliseconds.
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
	ft_printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms",
		bytes, g_ping.ip_str, seq, ttl, rtt);
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
 * sample around) via sqrt(sum2/n - avg^2). Only called when at least
 * one reply was received (see print_statistics()), so the division by
 * stats.received here is always safe.
 *
 * The subject explicitly exempts this line from needing to match
 * canonical's exact formatting.
 */
static void	print_rtt_line(void)
{
	double	avg;
	double	stddev;

	avg = g_ping.stats.rtt_sum / g_ping.stats.received;
	stddev = sqrt(g_ping.stats.rtt_sum2 / g_ping.stats.received
			- avg * avg);
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
 * line if at least one reply was received.
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
	if (g_ping.stats.received > 0)
		print_rtt_line();
}

/**
 * @brief Maps an ICMP error (type, code) pair to a human-readable
 * description, for the -v/--verbose error display.
 *
 * @param type An ICMP type value (already confirmed to be one of the
 *             recognised error types by is_icmp_error_type(), icmp.c).
 * @param code The type-specific code value.
 * @return A static description string, or NULL if this exact
 *         type/code combination isn't specifically named (the caller
 *         falls back to printing the raw numbers in that case).
 */
static const char	*icmp_error_desc(int type, int code)
{
	if (type == ICMP_DEST_UNREACH)
	{
		if (code == ICMP_NET_UNREACH)
			return ("Destination Net Unreachable");
		if (code == ICMP_HOST_UNREACH)
			return ("Destination Host Unreachable");
		if (code == ICMP_PROT_UNREACH)
			return ("Destination Protocol Unreachable");
		if (code == ICMP_PORT_UNREACH)
			return ("Destination Port Unreachable");
		return ("Destination Unreachable");
	}
	if (type == ICMP_SOURCE_QUENCH)
		return ("Source Quench");
	if (type == ICMP_REDIRECT)
		return ("Redirect");
	if (type == ICMP_TIME_EXCEEDED)
	{
		if (code == ICMP_EXC_FRAGTIME)
			return ("Frag reassembly time exceeded");
		return ("Time to live exceeded");
	}
	if (type == ICMP_PARAMETERPROB)
		return ("Parameter problem");
	return (NULL);
}

/**
 * @brief Prints one non-echo-reply ICMP error message, under
 * -v/--verbose.
 *
 * @param from_ip Numeric source address of the error message (never
 *                reverse-resolved - see the mandatory no-reverse-DNS
 *                requirement).
 * @param seq     The original request's sequence number this error
 *                refers to (from extract_original(), icmp.c).
 * @param type    The ICMP type of the error.
 * @param code    The ICMP code of the error.
 */
void	print_icmp_error(const char *from_ip, int seq, int type, int code)
{
	const char	*desc;

	desc = icmp_error_desc(type, code);
	if (desc)
		ft_printf("From %s icmp_seq=%d %s\n", from_ip, seq, desc);
	else
		ft_printf("From %s icmp_seq=%d ICMP type %d code %d\n",
			from_ip, seq, type, code);
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
