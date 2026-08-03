#include "ft_ping.h"

/*
** Standard Internet checksum (RFC 1071): sum all 16-bit words, fold
** any carry from the upper 16 bits back into the lower 16, then take
** the one's complement.
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

/*
** Fills the payload following the ICMP header: a timestamp (used to
** compute RTT on the way back) followed by a repeating byte pattern,
** matching canonical ping's default fill data.
*/
static void	fill_payload(unsigned char *packet)
{
	int	i;

	gettimeofday((struct timeval *)(packet + sizeof(struct icmphdr)), NULL);
	i = sizeof(struct icmphdr) + sizeof(struct timeval);
	while (i < g_ping.opts.packet_size)
	{
		packet[i] = (unsigned char)(i & 0xff);
		i++;
	}
}

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

static int	has_timestamp_payload(ssize_t bytes, int ip_hdr_len)
{
	return ((size_t)bytes >= (size_t)ip_hdr_len + sizeof(struct icmphdr)
		+ sizeof(struct timeval));
}

/*
** Builds and sends one ICMP echo request. Updates the transmitted
** counter and advances the sequence number on success.
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

/*
** Records a validated reply's RTT into the running statistics.
*/
static void	update_stats(double rtt)
{
	g_ping.stats.received++;
	if (g_ping.stats.received == 1 || rtt < g_ping.stats.rtt_min)
		g_ping.stats.rtt_min = rtt;
	if (rtt > g_ping.stats.rtt_max)
		g_ping.stats.rtt_max = rtt;
	g_ping.stats.rtt_sum += rtt;
	g_ping.stats.rtt_sum2 += rtt * rtt;
}

static int	is_icmp_error_type(int type)
{
	return (type == ICMP_DEST_UNREACH || type == ICMP_SOURCE_QUENCH
		|| type == ICMP_REDIRECT || type == ICMP_TIME_EXCEEDED
		|| type == ICMP_PARAMETERPROB);
}

/*
** ICMP error messages embed the original IP header + first 8 bytes of
** original datagram in payload (RFC 792)
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

/*
** Silently ignored unless -v is set
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

/*
** Waits for one ICMP packet, discarding anything that is not an echo
** reply matching our own pid (so replies destined for other ft_ping
** processes on the same host are ignored). Prints a temporary raw
** summary line; proper formatting arrives with output.c.
*/
int	receive_ping(void)
{
	unsigned char		buffer[MAX_PACKET];
	struct sockaddr_in	from;
	socklen_t			from_len;
	ssize_t				bytes;
	struct ip			*ip_hdr;
	struct icmphdr		*icmp_hdr;
	struct timeval		*sent_tv;
	struct timeval		now;
	int					ip_hdr_len;
	int					expected_seq;
	double				rtt;

	expected_seq = g_ping.seq - 1;
	while (1)
	{
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
			&& ntohs(icmp_hdr->un.echo.sequence) == (unsigned short)expected_seq
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
	update_stats(rtt);
	print_reply((int)(bytes - ip_hdr_len), ntohs(icmp_hdr->un.echo.sequence),
		ip_hdr->ip_ttl, rtt);
	return (0);
}
