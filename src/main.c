#include "ft_ping.h"

/*
** Global ping context, zero-initialised
*/
t_ping	g_ping;

static void	init_defaults(void)
{
	ft_memset(&g_ping, 0, sizeof(g_ping));
	g_ping.opts.ttl = DEFAULT_TTL;
	g_ping.opts.linger = DEFAULT_TIMEOUT;
	g_ping.opts.packet_size = PACKET_SIZE;
	g_ping.pid = getpid();
	g_ping.seq = 1;
	g_ping.stop = 0;
}

/*
** Paces the regular (non-preload) loop to one send per
** DEFAULT_INTERVAL. g_ping.seq counts every send including the
** preload burst, so preload is subtracted back out here -
** otherwise the burst's sequence numbers would read as if that
** many intervals had already elapsed, stalling the next send.
*/
static void	sleep_until_next_send(struct timeval *start_time)
{
	struct timeval	now;
	double			elapsed_ms;
	double			target_ms;
	double			wait_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(start_time, &now);
	target_ms = (double)(g_ping.seq - 1 - g_ping.opts.preload)
		* (double)DEFAULT_INTERVAL * 1000.0;
	if (g_ping.opts.timeout > 0
		&& elapsed_ms >= (double)g_ping.opts.timeout * 1000.0)
	{
		g_ping.stop = 1;
		return ;
	}
	if (elapsed_ms >= target_ms)
		return ;
	wait_ms = target_ms - elapsed_ms;
	if (g_ping.opts.timeout > 0)
	{
		if ((double)g_ping.opts.timeout * 1000.0 - elapsed_ms < wait_ms)
			wait_ms = (double)g_ping.opts.timeout * 1000.0 - elapsed_ms;
	}
	if (wait_ms > 0.0)
		usleep((useconds_t)(wait_ms * 1000.0));
	else
		g_ping.stop = 1;
}

/*
** -f/--flood pacing: caps the send rate at 100/sec (one every
** FLOOD_POLL_MS), measured from just before this iteration's send.
** wait_for_icmp_packet() already bounds the reply wait to the same
** window, so on a fast/lossless link (replies back in well under
** 10ms) this is what actually enforces the 100 pps ceiling; on a
** slower or lossy link elapsed_ms is already >= FLOOD_POLL_MS by the
** time we get here, so no extra sleep is added and sends track reply
** arrival instead, matching canonical's "as fast as they come back
** or 100 times a second, whichever is more".
*/
static void	sleep_flood_interval(struct timeval *send_time)
{
	struct timeval	now;
	double			elapsed_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(send_time, &now);
	if (elapsed_ms < (double)FLOOD_POLL_MS)
		usleep((useconds_t)(((double)FLOOD_POLL_MS - elapsed_ms) * 1000.0));
}

/*
** -l/--preload: fires `preload` echo requests back-to-back with no
** pacing, then drains that many replies before falling into the
** normal one-send-per-iteration loop below. Unlike canonical's
** interleaved async loop, each receive_ping() call here can block
** waiting on one specific reply; without a -w deadline to bound it,
** that would let a lossy target stall the drain for up to
** `preload * linger` seconds. So when the user hasn't set -w, the
** whole drain phase borrows the deadline machinery for one shared
** linger-second window instead of one per packet.
*/
static void	send_preload(void)
{
	int	i;
	int	saved_timeout;

	i = 0;
	while (i < g_ping.opts.preload)
	{
		send_ping();
		i++;
	}
	saved_timeout = g_ping.opts.timeout;
	if (g_ping.opts.timeout <= 0)
		g_ping.opts.timeout = g_ping.opts.linger;
	i = 0;
	while (i < g_ping.opts.preload && !g_ping.stop)
	{
		receive_ping();
		i++;
	}
	g_ping.opts.timeout = saved_timeout;
}

/*
** Entry point: sets defaults, parses arguments, resolves target,
** opens raw socket, installs SIGINT handler, sends one echo
** per request until interrupted, , printing banner, per-reply lines,
** and closing statistics.
*/
int	main(int argc, char **argv)
{
	struct timeval	now;
	struct timeval	send_time;
	int				first_send;

	init_defaults();
	parse_args(argc, argv);
	resolve_target(g_ping.target_raw);
	open_socket();
	signal(SIGINT, sigint_handler);
	print_start_banner();
	gettimeofday(&g_ping.start_time, NULL);
	send_preload();
	first_send = 1;
	while (!g_ping.stop)
	{
		if (g_ping.opts.timeout > 0)
		{
			gettimeofday(&now, NULL);
			if (timeval_diff_ms(&g_ping.start_time, &now)
					>= (double)g_ping.opts.timeout * 1000.0)
				break ;
		}
		gettimeofday(&send_time, NULL);
		if (send_ping() == 0 && g_ping.opts.flood && !first_send)
			ft_printf(".");
		first_send = 0;
		receive_ping();
		if (!g_ping.stop && g_ping.opts.flood)
			sleep_flood_interval(&send_time);
		else if (!g_ping.stop)
			sleep_until_next_send(&g_ping.start_time);
	}
	print_statistics();
	return (0);
}
