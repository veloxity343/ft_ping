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
** Millisecond difference between two timestamps, used to turn the
** timestamp embedded in an echo request's payload into an RTT once
** the matching reply comes back.
*/
static double	diff_ms(struct timeval *start, struct timeval *end)
{
	double	sec_diff;
	double	usec_diff;

	sec_diff = (double)(end->tv_sec - start->tv_sec) * 1000.0;
	usec_diff = (double)(end->tv_usec - start->tv_usec) / 1000.0;
	return (sec_diff + usec_diff);
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

/*
** Builds and sends one ICMP echo request. Updates the transmitted
** counter and advances the sequence number on success.
*/
int	send_ping(void)
{
	unsigned char	packet[MAX_PACKET];
	struct icmphdr	*icmp_hdr;

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
	double				rtt;

	from_len = sizeof(from);
	bytes = recvfrom(g_ping.sockfd, buffer, sizeof(buffer), 0,
			(struct sockaddr *)&from, &from_len);
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (-1);
		ft_printf("%s: recvfrom: %s\n", PROG_NAME, strerror(errno));
		return (-1);
	}
	ip_hdr = (struct ip *)buffer;
	ip_hdr_len = ip_hdr->ip_hl * 4;
	if ((size_t)bytes < (size_t)ip_hdr_len + sizeof(struct icmphdr))
		return (-1);
	icmp_hdr = (struct icmphdr *)(buffer + ip_hdr_len);
	if (icmp_hdr->type != ICMP_ECHOREPLY)
		return (-1);
	if (ntohs(icmp_hdr->un.echo.id) != (unsigned short)g_ping.pid)
		return (-1);
	sent_tv = (struct timeval *)(buffer + ip_hdr_len + sizeof(struct icmphdr));
	gettimeofday(&now, NULL);
	rtt = diff_ms(sent_tv, &now);
	update_stats(rtt);
	ft_printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
		(int)(bytes - ip_hdr_len), g_ping.ip_str,
		ntohs(icmp_hdr->un.echo.sequence), ip_hdr->ip_ttl, rtt);
	return (0);
}
