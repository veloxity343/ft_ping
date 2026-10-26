#include "ft_ping.h"

/**
 * @brief Internet checksum (RFC 1071) over len bytes of buf.
 */
unsigned short	icmp_checksum(void *buf, int len)
{
	unsigned short	*buf16;
	unsigned int	sum;

	buf16 = (unsigned short *)buf;
	sum = 0;
	while (len > 1)
	{
		sum += *buf16++;
		len -= 2;
	}
	if (len == 1)
		sum += *(unsigned char *)buf16;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ((unsigned short)~sum);
}

/**
 * @brief Fills the payload after the ICMP header: the send time first (if
 *        it fits), then the -p pattern or an incrementing byte.
 */
static void	fill_payload(unsigned char *packet)
{
	int	i;
	int	pattern_idx;

	i = sizeof(struct icmphdr);
	if (TIMING(g_ping.opts.packet_size))
	{
		gettimeofday((struct timeval *)(packet + i), NULL);
		i += sizeof(struct timeval);
	}
	pattern_idx = 0;
	while (i < g_ping.opts.packet_size)
	{
		if (g_ping.opts.pattern_set && g_ping.opts.pattern_len > 0)
		{
			packet[i] = g_ping.opts.pattern[pattern_idx];
			pattern_idx++;
			if (pattern_idx >= g_ping.opts.pattern_len)
				pattern_idx = 0;
		}
		else
			packet[i] = (unsigned char)(i & 0xff);
		i++;
	}
}

/**
 * @brief Checks the packet is long enough for an IP and an ICMP header, and
 *        points ip_hdr and icmp_hdr at them.
 */
static int	validate_ip_icmp_packet(const unsigned char *buffer, ssize_t bytes,
		struct ip **ip_hdr, int *ip_hdr_len, struct icmphdr **icmp_hdr)
{
	if (bytes < (ssize_t)sizeof(struct ip))
		return (0);
	*ip_hdr = (struct ip *)buffer;
	*ip_hdr_len = (*ip_hdr)->ip_hl * 4;
	if (*ip_hdr_len < (int)sizeof(struct ip))
		return (0);
	if ((size_t)bytes < (size_t)(*ip_hdr_len) + sizeof(struct icmphdr))
		return (0);
	*icmp_hdr = (struct icmphdr *)(buffer + *ip_hdr_len);
	return (1);
}

/**
 * @brief Tells whether a reply is long enough to still hold our send
 *        timestamp.
 */
static int	has_timestamp_payload(ssize_t bytes, int ip_hdr_len)
{
	return ((size_t)bytes >= (size_t)ip_hdr_len + sizeof(struct icmphdr)
		+ sizeof(struct timeval));
}

/**
 * @brief Reads a big-endian 32-bit value from packet bytes.
 */
static unsigned int	read_be32(const unsigned char *p)
{
	unsigned int	val;

	ft_memcpy(&val, p, 4);
	return (ntohl(val));
}

/**
 * @brief Reads 32 bits from packet bytes as they are, with no byte swap.
 */
static unsigned int	read_raw32(const unsigned char *p)
{
	unsigned int	val;

	ft_memcpy(&val, p, 4);
	return (val);
}

/**
 * @brief Pulls the IP Timestamp option (RFC 791) out of a reply's IP header
 *        into out, if there is one.
 */
static void	parse_ip_timestamp_reply(const unsigned char *ip_hdr,
		int ip_hdr_len, t_ts_option *out)
{
	int				pos;
	int				opt_len;
	int				slot_size;
	int				filled;
	const unsigned char	*data;

	ft_memset(out, 0, sizeof(*out));
	pos = (int)sizeof(struct ip);
	while (pos < ip_hdr_len)
	{
		if (ip_hdr[pos] == 0)
			break ;
		if (ip_hdr[pos] == 1)
		{
			pos++;
			continue ;
		}
		if (pos + 1 >= ip_hdr_len)
			break ;
		opt_len = ip_hdr[pos + 1];
		if (opt_len < 4 || pos + opt_len > ip_hdr_len)
			break ;
		if (ip_hdr[pos] == IPOPT_TS)
		{
			out->present = 1;
			out->tsaddr = ((ip_hdr[pos + 3] & 0x0f) == 1);
			out->overflow = (ip_hdr[pos + 3] >> 4) & 0x0f;
			slot_size = 4 + out->tsaddr * 4;
			filled = (ip_hdr[pos + 2] - 5) / slot_size;
			if (filled > (opt_len - 4) / slot_size)
				filled = (opt_len - 4) / slot_size;
			if (filled > 9)
				filled = 9;
			data = ip_hdr + pos + 4;
			while (filled > 0 && out->count < filled)
			{
				if (out->tsaddr)
				{
					out->addrs[out->count] = read_raw32(data);
					data += 4;
				}
				out->times[out->count] = read_be32(data);
				data += 4;
				out->count++;
			}
			return ;
		}
		pos += opt_len;
	}
}

/**
 * @brief Returns 1 if a reply for this sequence number was already counted.
 */
static int	seq_seen(unsigned short seq)
{
	return ((g_ping.dup_seen[seq / 8] & (1 << (seq % 8))) != 0);
}

/**
 * @brief Records that a reply for this sequence number has been counted.
 */
static void	seq_mark(unsigned short seq)
{
	g_ping.dup_seen[seq / 8] |= (1 << (seq % 8));
}

/**
 * @brief Waits with poll() for a packet. The -W window counts from began
 *        (the start of receive_ping), so unrelated packets can't extend it,
 *        and it never runs past the -w deadline. Returns >0 if readable,
 *        0 on timeout, -1 on error or signal.
 */
static int	wait_for_icmp_packet(struct timeval *began)
{
	struct pollfd	pfd;
	struct timeval	now;
	double			wait_ms;
	double			remaining_ms;
	int				timeout_ms;
	int				rc;

	wait_ms = (double)g_ping.opts.linger * 1000.0;
	if (g_ping.opts.flood && wait_ms > (double)FLOOD_POLL_MS)
		wait_ms = (double)FLOOD_POLL_MS;
	gettimeofday(&now, NULL);
	wait_ms -= timeval_diff_ms(began, &now);
	if (wait_ms <= 0.0)
		return (0);
	if (g_ping.opts.timeout > 0)
	{
		remaining_ms = (double)g_ping.opts.timeout * 1000.0
			- timeval_diff_ms(&g_ping.start_time, &now);
		if (remaining_ms <= 0.0)
			return (0);
		if (remaining_ms < wait_ms)
			wait_ms = remaining_ms;
	}
	timeout_ms = (int)ceil(wait_ms);
	pfd.fd = g_ping.sockfd;
	pfd.events = POLLIN;
	while ((rc = poll(&pfd, 1, timeout_ms)) < 0)
	{
		if (errno == EINTR)
			return (-1);
		ft_printf("%s: poll: %s\n", PROG_NAME, strerror(errno));
		return (-1);
	}
	return (rc);
}

/**
 * @brief Builds and sends one echo request, then bumps the sent count and
 *        sequence number. Returns -1 if sendto() fails.
 */
int	send_ping(void)
{
	unsigned char	packet[MAX_PACKET];
	struct icmphdr	*icmp_hdr;

	if ((unsigned short)g_ping.seq == 0)
		ft_memset(g_ping.dup_seen, 0, sizeof(g_ping.dup_seen));
	ft_memset(packet, 0, g_ping.opts.packet_size);
	icmp_hdr = (struct icmphdr *)packet;
	icmp_hdr->type = ICMP_ECHO;
	icmp_hdr->code = 0;
	icmp_hdr->un.echo.id = htons((unsigned short)g_ping.pid);
	icmp_hdr->un.echo.sequence = htons((unsigned short)g_ping.seq);
	icmp_hdr->checksum = 0;
	fill_payload(packet);
	icmp_hdr->checksum = icmp_checksum(packet, g_ping.opts.packet_size);
	if (sendto(g_ping.sockfd, packet, g_ping.opts.packet_size, 0,
			(struct sockaddr *)&g_ping.dest_addr,
			sizeof(g_ping.dest_addr)) < 0)
	{
		ft_printf("%s: sendto: %s\n", PROG_NAME, strerror(errno));
		return (-1);
	}
	g_ping.stats.transmitted++;
	g_ping.seq++;
	return (0);
}

/**
 * @brief Adds one RTT sample to the min, max and sums behind the round-trip
 *        line.
 */
static void	update_rtt_stats(double rtt)
{
	if (g_ping.stats.rtt_count == 0 || rtt < g_ping.stats.rtt_min)
		g_ping.stats.rtt_min = rtt;
	if (rtt > g_ping.stats.rtt_max)
		g_ping.stats.rtt_max = rtt;
	g_ping.stats.rtt_sum += rtt;
	g_ping.stats.rtt_sum2 += rtt * rtt;
	g_ping.stats.rtt_count++;
}

/**
 * @brief Returns 1 for the ICMP error types we report.
 */
static int	is_icmp_error_type(int type)
{
	return (type == ICMP_DEST_UNREACH || type == ICMP_SOURCE_QUENCH
		|| type == ICMP_REDIRECT || type == ICMP_TIME_EXCEEDED
		|| type == ICMP_PARAMETERPROB);
}

/**
 * @brief Checks that the datagram quoted in an ICMP error is one of our own
 *        echo requests (same id).
 */
static int	is_our_original(const unsigned char *payload, int payload_len)
{
	const struct ip		*inner_ip;
	const struct icmphdr	*inner_icmp;
	int					inner_ip_len;

	if (payload_len < (int)sizeof(struct ip))
		return (0);
	inner_ip = (const struct ip *)payload;
	inner_ip_len = inner_ip->ip_hl * 4;
	if (inner_ip_len < (int)sizeof(struct ip)
		|| payload_len < inner_ip_len + (int)sizeof(struct icmphdr))
		return (0);
	inner_icmp = (const struct icmphdr *)(payload + inner_ip_len);
	if (inner_ip->ip_p != IPPROTO_ICMP
		|| ntohs(inner_icmp->un.echo.id) != (unsigned short)g_ping.pid)
		return (0);
	return (1);
}

/**
 * @brief Prints an ICMP error if it is about one of our requests. Without
 *        -v, only errors for packets sent to the target are shown.
 */
static void	handle_icmp_error(struct icmphdr *icmp_hdr, unsigned char *buffer,
		int bytes, int ip_hdr_len, struct sockaddr_in *from)
{
	const unsigned char	*orig;
	int					orig_len;
	struct ip			inner_ip;
	char				from_ip[INET_ADDRSTRLEN];

	orig = buffer + ip_hdr_len + sizeof(struct icmphdr);
	orig_len = bytes - ip_hdr_len - (int)sizeof(struct icmphdr);
	if (!is_our_original(orig, orig_len))
		return ;
	ft_memcpy(&inner_ip, orig, sizeof(inner_ip));
	if (!g_ping.opts.verbose
		&& inner_ip.ip_dst.s_addr != g_ping.dest_addr.sin_addr.s_addr)
		return ;
	inet_ntop(AF_INET, &from->sin_addr, from_ip, sizeof(from_ip));
	print_icmp_error(from_ip, bytes - ip_hdr_len, icmp_hdr, orig);
}

/**
 * @brief Waits for one echo reply meant for us, updates the stats and
 *        prints it, handling ICMP errors on the way. Returns 0 on a reply, -1 on
 *        timeout, signal or socket error.
 */
int	receive_ping(void)
{
	unsigned char		buffer[MAX_RECV_PACKET];
	struct sockaddr_in	from;
	socklen_t			from_len;
	ssize_t				bytes;
	struct ip			*ip_hdr;
	struct icmphdr		*icmp_hdr;
	struct timeval		sent_tv;
	struct timeval		now;
	struct timeval		began;
	int					ip_hdr_len;
	int					seq;
	int					dup;
	double				rtt;
	t_ts_option			ts;

	gettimeofday(&began, NULL);
	while (1)
	{
		if (g_ping.stop || wait_for_icmp_packet(&began) <= 0)
			return (-1);
		from_len = sizeof(from);
		bytes = recvfrom(g_ping.sockfd, buffer, sizeof(buffer), 0,
				(struct sockaddr *)&from, &from_len);
		if (bytes < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				return (-1);
			ft_printf("%s: recvfrom: %s\n", PROG_NAME, strerror(errno));
			return (-1);
		}
		if (!validate_ip_icmp_packet(buffer, bytes, &ip_hdr, &ip_hdr_len,
				&icmp_hdr))
			continue;
		if (icmp_hdr->type != ICMP_ECHOREPLY)
		{
			if (is_icmp_error_type(icmp_hdr->type))
				handle_icmp_error(icmp_hdr, buffer, bytes, ip_hdr_len, &from);
			continue ;
		}
		if (ntohs(icmp_hdr->un.echo.id) == (unsigned short)g_ping.pid
			&& from.sin_addr.s_addr == g_ping.dest_addr.sin_addr.s_addr)
		{
			if (TIMING(g_ping.opts.packet_size)
				&& !has_timestamp_payload(bytes, ip_hdr_len))
				continue ;
			break ;
		}
	}
	/* Like inetutils, a reply's checksum isn't checked (it only warns), and
	** the RTT is taken when we read the reply, from the wall clock. */
	rtt = -1.0;
	if (TIMING(g_ping.opts.packet_size))
	{
		ft_memcpy(&sent_tv, buffer + ip_hdr_len + sizeof(struct icmphdr),
			sizeof(sent_tv));
		gettimeofday(&now, NULL);
		rtt = timeval_diff_ms(&sent_tv, &now);
	}
	seq = ntohs(icmp_hdr->un.echo.sequence);
	if (rtt >= 0.0)
		update_rtt_stats(rtt);
	dup = seq_seen((unsigned short)seq);
	if (dup)
		g_ping.stats.duplicates++;
	else
	{
		seq_mark((unsigned short)seq);
		g_ping.stats.received++;
	}
	print_reply((int)(bytes - ip_hdr_len), seq, ip_hdr->ip_ttl, rtt, dup);
	if (g_ping.opts.ip_timestamp_set)
	{
		parse_ip_timestamp_reply(buffer, ip_hdr_len, &ts);
		print_ip_timestamp_option(&ts);
	}
	return (0);
}
