#include "ft_ping.h"

/*
** Printed once before the send/receive loop starts. Data size is the
** payload size (packet_size minus the ICMP header)
*/
void	print_start_banner(void)
{
	ft_printf("PING %s (%s): %d data bytes\n", g_ping.hostname,
		g_ping.ip_str, g_ping.opts.packet_size - (int)sizeof(struct icmphdr));
}

/*
** Per-reply line
*/
void	print_reply(int bytes, int seq, int ttl, double rtt)
{
	ft_printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
		bytes, g_ping.ip_str, seq, ttl, rtt);
}

/*
** Packet loss percentage as a whole number, truncated
*/
static int	packet_loss_pct(void)
{
	if (g_ping.stats.transmitted == 0)
		return (0);
	return (((g_ping.stats.transmitted - g_ping.stats.received) * 100)
		/ g_ping.stats.transmitted);
}

/*
** rtt min/avg/max/mdev line. mdev is standard deviation of
** observed RTTs, computed from running sum/sum-of-squares via
** sqrt(sum2/n - avg^2)
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

/*
** Final summary block, printed once loop stops
*/
void	print_statistics(void)
{
	ft_printf("\n--- %s ping statistics ---\n", g_ping.hostname);
	ft_printf("%d packets transmitted, %d packets received, %d%% "
		"packet loss\n", g_ping.stats.transmitted,
		g_ping.stats.received, packet_loss_pct());
	if (g_ping.stats.received > 0)
		print_rtt_line();
}

/*
** Fall-through description for ICMP error types and codes
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

/*
** Print for ICMP -v errors
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
