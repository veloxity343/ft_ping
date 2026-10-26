#include "ft_ping.h"

/**
 * @brief Prints the PING line: host, ip, data bytes, and the id under -v.
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
 * @brief Prints one reply line, with time= only if it was timed and (DUP!)
 *        for duplicates. Under -f it is just a backspace.
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
 * @brief Packet loss as a whole percent, truncated.
 */
static int	packet_loss_pct(void)
{
	if (g_ping.stats.transmitted == 0)
		return (0);
	return (((g_ping.stats.transmitted - g_ping.stats.received) * 100)
		/ g_ping.stats.transmitted);
}

/**
 * @brief Prints the round-trip min/avg/max/stddev line.
 */
static void	print_rtt_line(void)
{
	double	n;
	double	avg;
	double	var;
	double	stddev;

	n = g_ping.stats.rtt_count;
	avg = g_ping.stats.rtt_sum / n;
	var = g_ping.stats.rtt_sum2 / n - avg * avg;
	if (var < 0.0)
		var = 0.0;
	stddev = sqrt(var);
	ft_printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
		g_ping.stats.rtt_min, avg, g_ping.stats.rtt_max, stddev);
}

/**
 * @brief Prints the closing statistics block. The round-trip line is only
 *        printed if some replies were timed.
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
	if (g_ping.stats.rtt_count > 0)
		print_rtt_line();
}

/**
 * @brief An ICMP type/code pair and its message text.
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
 * @brief Returns the message for an ICMP type/code, or NULL if it isn't
 *        listed.
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
 * @brief Prints the IP header quoted in an ICMP error: a hex dump under -v,
 *        then the labelled field row.
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
	/* Same byte-order guess as inetutils, so Len matches it even where it
	** looks odd (1c00 for a 28 byte packet). Kept on purpose. */
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
 * @brief Prints the request quoted in an ICMP error: its IP header, then a
 *        line for its TCP, UDP or ICMP content.
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
 * @brief Prints an ICMP error as '<n> bytes from <addr>: <text>', plus the
 *        quoted request under -v.
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
 * @brief Prints one IP timestamp in ms, in angle brackets if the
 *        non-standard clock bit is set.
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
 * @brief Prints a tsaddr slot's address as name (ip) when reverse DNS works
 *        and -n isn't set, otherwise just the ip.
 */
static void	print_ts_addr(unsigned int raw_addr)
{
	struct in_addr	addr;
	char			host[256];

	addr.s_addr = raw_addr;
	ft_printf("\t");
	/* The lookup can block on a slow DNS, as in inetutils. -n skips it. */
	if (resolve_reverse(addr, host, sizeof(host)))
		ft_printf("%s (%s)", host, inet_ntoa(addr));
	else
		ft_printf("%s", inet_ntoa(addr));
}

/**
 * @brief Prints the TS: block for a reply that carried an IP timestamp
 *        option.
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
