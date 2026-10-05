#include "ft_ping.h"

/**
 * @brief Standard Internet checksum (RFC 1071).
 *
 * Sums all 16-bit words in buf, folds any carry out of the upper 16 bits
 * back into the lower 16 (possibly more than once, hence the second fold
 * after the first), then takes the one's complement. Used both to
 * compute the outgoing ICMP checksum (send_ping()) and implicitly
 * relied on by the kernel to validate incoming ones (this program never
 * re-checks a reply's checksum itself).
 *
 * @param buf Pointer to the data to checksum (the whole ICMP packet).
 * @param len Length of buf in bytes; the trailing odd byte, if any, is
 *            summed on its own.
 * @return The 16-bit one's-complement checksum.
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
 * @brief Fills the ICMP payload (everything after the 8-byte header).
 *
 * The first sizeof(struct timeval) bytes are the current time, written
 * in place via gettimeofday() directly into the packet buffer - this is
 * read back on the matching reply to compute RTT (see receive_ping()).
 * Everything after that is either the user's -p/--pattern bytes, cycled
 * to fill the remaining space, or (default) an incrementing byte value,
 * matching canonical ping's default fill data.
 *
 * @param packet The packet buffer, already sized to g_ping.opts.
 *               packet_size by the caller (send_ping()).
 */
static void	fill_payload(unsigned char *packet)
{
	int	i;
	int	pattern_idx;

	gettimeofday((struct timeval *)(packet + sizeof(struct icmphdr)), NULL);
	i = sizeof(struct icmphdr) + sizeof(struct timeval);
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
 * @brief Validates that a received packet is at least large enough to
 * hold a well-formed IP header and an ICMP header, and locates both
 * within the buffer.
 *
 * @param buffer      The raw bytes read by recvfrom().
 * @param bytes       Number of bytes actually received.
 * @param ip_hdr      Out: set to point at the start of buffer.
 * @param ip_hdr_len  Out: the IP header's declared length (ip_hl * 4),
 *                    options included.
 * @param icmp_hdr    Out: set to point just past the IP header.
 * @return 1 if buffer is large enough and ip_hdr_len is internally
 *         consistent, else 0 (out parameters are only valid on success).
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
 * @brief Checks whether a received packet is large enough to contain the
 * embedded struct timeval this program's own echo requests always send
 * (see fill_payload()) - i.e. whether an RTT can actually be computed
 * from it.
 *
 * @param bytes      Number of bytes actually received.
 * @param ip_hdr_len The IP header length, as found by
 *                   validate_ip_icmp_packet().
 * @return 1 if there's room for the timestamp payload, else 0.
 */
static int	has_timestamp_payload(ssize_t bytes, int ip_hdr_len)
{
	return ((size_t)bytes >= (size_t)ip_hdr_len + sizeof(struct icmphdr)
		+ sizeof(struct timeval));
}

/**
 * @brief Reads a 32-bit big-endian (network byte order) value out of raw
 * packet bytes and converts it to a host-order integer.
 *
 * Uses memcpy rather than a pointer cast, since p is not guaranteed to be
 * 4-byte aligned (it points into an arbitrary offset within a received
 * packet buffer) and an unaligned cast+dereference is undefined behaviour.
 *
 * @param p Pointer to 4 raw bytes, network byte order.
 * @return The value as a host-order unsigned int.
 */
static unsigned int	read_be32(const unsigned char *p)
{
	unsigned int	val;

	ft_memcpy(&val, p, 4);
	return (ntohl(val));
}

/**
 * @brief Reads a raw 32-bit value out of packet bytes without any
 * byte-order conversion.
 *
 * Used for IPv4 addresses recorded in a tsaddr timestamp option: they need
 * to stay in network byte order for inet_ntoa()/getnameinfo(), unlike the
 * timestamps read alongside them (see read_be32()), which need host order
 * for arithmetic.
 *
 * @param p Pointer to 4 raw bytes.
 * @return Those 4 bytes reassembled into an unsigned int, order preserved.
 */
static unsigned int	read_raw32(const unsigned char *p)
{
	unsigned int	val;

	ft_memcpy(&val, p, 4);
	return (val);
}

/**
 * @brief Extracts an IP Timestamp option (RFC 791, type 68) from a
 * received reply's IP header, if present.
 *
 * Walks the IP header's option bytes as a standard TLV list (skipping NOP
 * padding bytes and any other option type by its declared length) looking
 * for IPOPT_TS. Every offset is checked against ip_hdr_len before it is
 * read, since this parses network-controlled data that could be malformed
 * or truncated - a well-formed reply to our own request is the expected
 * case, but nothing here trusts that assumption.
 *
 * The option's pointer byte (1-relative offset of the next free slot)
 * tells us how many slots got filled in transit; that count is converted
 * to a 0-relative slot index and clamped to both the option's own declared
 * length and the fixed-size arrays in t_ts_option.
 *
 * @param ip_hdr     Pointer to the start of the received IP header.
 * @param ip_hdr_len Total IP header length in bytes, options included.
 *                    Already validated by the caller (validate_ip_icmp_
 *                    packet()) to not exceed the actual bytes received.
 * @param out        Populated with the decoded option. out->present is 0
 *                    if no timestamp option was found or it was malformed.
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
 * @brief Checks whether a reply for this sequence number has already
 * been counted, via g_ping.dup_seen.
 *
 * One bit per possible 16-bit ICMP sequence number, set by seq_mark()
 * the first time a reply for that seq is matched. Correct regardless of
 * arrival order, unlike a monotonic "next expected seq" pointer, which
 * breaks once multiple requests can be outstanding at once (preload/
 * flood) and a later seq's reply arrives before an earlier one's - an
 * earlier design here used exactly such a pointer and had to be
 * reverted for that reason.
 *
 * @param seq The reply's ICMP sequence number, host byte order.
 * @return 1 if this is a duplicate (bit already set), 0 if this is the
 *         first reply seen for seq.
 */
static int	seq_seen(unsigned short seq)
{
	return ((g_ping.dup_seen[seq / 8] & (1 << (seq % 8))) != 0);
}

/**
 * @brief Marks a sequence number as having been matched, so a later
 * duplicate reply for the same seq is caught by seq_seen().
 *
 * @param seq The reply's ICMP sequence number, host byte order.
 */
static void	seq_mark(unsigned short seq)
{
	g_ping.dup_seen[seq / 8] |= (1 << (seq % 8));
}

/**
 * @brief Blocks (via poll()) until either an ICMP packet is available on
 * the socket, the applicable wait budget expires, or a signal
 * interrupts the wait.
 *
 * The wait budget is normally -W/--linger seconds, capped to
 * FLOOD_POLL_MS (10ms) under -f/--flood so a single lost packet can't
 * stall the 100pps send cadence (see sleep_flood_interval(), main.c).
 * It is further clamped to whatever remains of the -w/--timeout deadline,
 * if one is set, so this never blocks past it.
 *
 * @return >0 if the socket is readable, 0 on timeout (nothing arrived
 *         within the wait budget), -1 on error or on being interrupted
 *         by a signal (e.g. SIGINT).
 */
static int	wait_for_icmp_packet(void)
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
	if (g_ping.opts.timeout > 0)
	{
		gettimeofday(&now, NULL);
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
 * @brief Builds and sends one ICMP echo request.
 *
 * Sets id = our PID (truncated to 16 bits - this is how replies are
 * later matched back to us, see receive_ping()) and sequence = the
 * current g_ping.seq. Whenever g_ping.seq is about to wrap the 16-bit
 * ICMP sequence field back to 0 (i.e. a new "lap" of the sequence space
 * begins, relevant to runs exceeding 65536 packets, e.g. a large -l),
 * the duplicate-tracking bitmap is cleared first - otherwise a
 * legitimate new reply reusing an old sequence number would be
 * misread as a duplicate of a reply from many packets ago.
 *
 * On success, increments g_ping.stats.transmitted and g_ping.seq.
 *
 * @return 0 on success, -1 on failure (packet size misconfigured, or
 *         sendto() failed) - stats/seq are left untouched on failure.
 */
int	send_ping(void)
{
	unsigned char	packet[MAX_PACKET];
	struct icmphdr	*icmp_hdr;

	if (g_ping.opts.packet_size < MIN_PACKET_SIZE)
	{
		ft_printf("%s: packet size too small\n", PROG_NAME);
		return (-1);
	}
	if ((unsigned short)g_ping.seq == 0)
		ft_memset(g_ping.dup_seen, 0, sizeof(g_ping.dup_seen));
	ft_memset(packet, 0, sizeof(packet));
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
 * @brief Records a validated reply's RTT into the running min/max/sum
 * statistics used to build the closing round-trip line.
 *
 * Called for every matched reply, duplicates included, matching
 * canonical (which times a reply before checking whether it's a dup).
 * Counting into stats.received/duplicates happens separately in the
 * caller (receive_ping()) - this function only ever touches the RTT
 * fields.
 *
 * @param rtt Round-trip time in milliseconds for this reply.
 */
static void	update_rtt_stats(double rtt)
{
	if (g_ping.stats.received + g_ping.stats.duplicates == 0
		|| rtt < g_ping.stats.rtt_min)
		g_ping.stats.rtt_min = rtt;
	if (rtt > g_ping.stats.rtt_max)
		g_ping.stats.rtt_max = rtt;
	g_ping.stats.rtt_sum += rtt;
	g_ping.stats.rtt_sum2 += rtt * rtt;
}

/**
 * @brief Checks whether an ICMP type is one of the non-echo-reply error
 * messages this program reports under -v/--verbose (see
 * handle_icmp_error()).
 *
 * @param type An ICMP type value from a received packet's header.
 * @return 1 if type is a recognised error type, else 0.
 */
static int	is_icmp_error_type(int type)
{
	return (type == ICMP_DEST_UNREACH || type == ICMP_SOURCE_QUENCH
		|| type == ICMP_REDIRECT || type == ICMP_TIME_EXCEEDED
		|| type == ICMP_PARAMETERPROB);
}

/**
 * @brief Extracts our own sequence number from an ICMP error message's
 * embedded copy of the original request.
 *
 * Per RFC 792, ICMP error messages (destination unreachable, time
 * exceeded, etc.) embed the original IP header plus the first 8 bytes of
 * the original datagram - enough to contain our own ICMP echo header,
 * letting us confirm the error really is about one of our own requests
 * (by id) and report which one (by sequence).
 *
 * @param payload     Pointer to the start of the embedded original
 *                    datagram, inside the received error packet.
 * @param payload_len Bytes available at payload (bounded by how much of
 *                    the original packet the error message included).
 * @param seq         Out: set to the original request's sequence number
 *                    on success.
 * @return 1 if payload contained a recognisable copy of one of our own
 *         echo requests (matching id), else 0.
 */
static int	extract_original(unsigned char *payload, int payload_len,
		int *seq)
{
	struct ip		*inner_ip;
	struct icmphdr	*inner_icmp;
	int				inner_ip_len;

	if (payload_len < (int)sizeof(struct ip))
		return (0);
	inner_ip = (struct ip *)payload;
	inner_ip_len = inner_ip->ip_hl * 4;
	if (payload_len < inner_ip_len + (int)sizeof(struct icmphdr))
		return (0);
	inner_icmp = (struct icmphdr *)(payload + inner_ip_len);
	if (ntohs(inner_icmp->un.echo.id) != (unsigned short)g_ping.pid)
		return (0);
	*seq = ntohs(inner_icmp->un.echo.sequence);
	return (1);
}

/**
 * @brief Reports one non-echo-reply ICMP message, if -v/--verbose is
 * set and it can be confirmed to be about one of our own requests.
 *
 * This is the mandatory-requirement behaviour: "-v ... also allow us to
 * see the results in case of a problem or error linked to the packets,
 * which logically shouldn't force the program to stop." Silently
 * ignored (returns immediately) unless -v is set; the caller
 * (receive_ping()) always continues its wait loop afterwards regardless,
 * so an error reply never halts the program.
 *
 * @param icmp_hdr   The received ICMP header (an error type, not an
 *                   echo reply - the caller already checked that).
 * @param buffer     The full received packet buffer.
 * @param bytes      Total bytes received.
 * @param ip_hdr_len Length of the outer IP header, so the embedded
 *                   original datagram can be located just past it.
 * @param from       The error message's source address, for display.
 */
static void	handle_icmp_error(struct icmphdr *icmp_hdr, unsigned char *buffer,
		int bytes, int ip_hdr_len, struct sockaddr_in *from)
{
	int		seq;
	char	from_ip[INET_ADDRSTRLEN];

	if (!g_ping.opts.verbose)
		return ;
	if (!extract_original(buffer + ip_hdr_len + sizeof(struct icmphdr),
			bytes - ip_hdr_len - (int)sizeof(struct icmphdr), &seq))
		return ;
	inet_ntop(AF_INET, &from->sin_addr, from_ip, sizeof(from_ip));
	print_icmp_error(from_ip, seq, icmp_hdr->type, icmp_hdr->code);
}

/**
 * @brief Waits for and processes one matching ICMP echo reply.
 *
 * Loops on wait_for_icmp_packet()/recvfrom(), discarding anything that
 * isn't a validly-sized echo reply from our target matching our own PID
 * (via the ICMP id field) - this is how replies meant for other ft_ping
 * processes on the same host are ignored, and also routes non-echo-reply
 * ICMP messages (destination unreachable, TTL exceeded, etc.) to
 * handle_icmp_error() along the way rather than silently dropping them.
 *
 * Reply matching does not check sequence number against "the packet we
 * just sent": any valid reply for our PID is accepted, which is
 * necessary once multiple requests can be outstanding at once under
 * -l/-f. Duplicate detection (seq_seen()/seq_mark()) is what
 * distinguishes a genuinely new reply from a repeat of one already
 * counted.
 *
 * On a successful match: records the RTT, updates
 * stats.received/duplicates, prints the reply line (print_reply()), and
 * - if --ip-timestamp was requested - parses and prints any IP
 * Timestamp option the reply carried.
 *
 * @return 0 if a reply was matched and processed, -1 on timeout, a
 *         signal interrupting the wait, or a socket error.
 */
int	receive_ping(void)
{
	unsigned char		buffer[MAX_RECV_PACKET];
	struct sockaddr_in	from;
	socklen_t			from_len;
	ssize_t				bytes;
	struct ip			*ip_hdr;
	struct icmphdr		*icmp_hdr;
	struct timeval		*sent_tv;
	struct timeval		now;
	int					ip_hdr_len;
	int					seq;
	int					dup;
	double				rtt;
	t_ts_option			ts;

	while (1)
	{
		if (wait_for_icmp_packet() <= 0)
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
			if (!has_timestamp_payload(bytes, ip_hdr_len))
				continue ;
			break ;
		}
	}
	sent_tv = (struct timeval *)(buffer + ip_hdr_len + sizeof(struct icmphdr));
	gettimeofday(&now, NULL);
	rtt = timeval_diff_ms(sent_tv, &now);
	seq = ntohs(icmp_hdr->un.echo.sequence);
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
