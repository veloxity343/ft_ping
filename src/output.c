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
	double	mdev;

	avg = g_ping.stats.rtt_sum / g_ping.stats.received;
	mdev = sqrt(g_ping.stats.rtt_sum2 / g_ping.stats.received
			- avg * avg);
	ft_printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
		g_ping.stats.rtt_min, avg, g_ping.stats.rtt_max, mdev);
}

/*
** Final summary block, printed once loop stops
*/
void	print_statistics(void)
{
	struct timeval	now;
	double			elapsed;

	gettimeofday(&now, NULL);
	elapsed = timeval_diff_ms(&g_ping.start_time, &now);
	ft_printf("\n--- %s ft_ping statistics ---\n", g_ping.hostname);
	ft_printf("%d packets transmitted, %d received, %d%% packet loss, "
		"time %dms\n", g_ping.stats.transmitted, g_ping.stats.received,
		packet_loss_pct(), (int)elapsed);
	if (g_ping.stats.received > 0)
		print_rtt_line();
}
