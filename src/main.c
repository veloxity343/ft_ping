#include "ft_ping.h"

/**
 * @brief Global ping state, shared by every file.
 */
t_ping	g_ping;

/**
 * @brief Zeroes g_ping and sets the non-zero defaults (TTL, linger, packet
 *        size, pid, first sequence number).
 */
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

/**
 * @brief Sleeps until the next once-a-second send is due, or stops the run
 *        if the -w deadline comes first.
 */
static void	sleep_until_next_send(struct timeval *start_time)
{
	struct timeval	now;
	double			elapsed_ms;
	double			target_ms;
	double			wait_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(start_time, &now);
	/* seq only moves on a successful send. If sends keep failing the loop
	** doesn't spin anyway, receive_ping() still waits out the -W window. */
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

/**
 * @brief Flood pacing: waits out what is left of the 10 ms slot since
 *        send_time (at most 100 packets a second).
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

/**
 * @brief Sends the -l burst back to back, then reads that many replies
 *        (within one -W window if no -w is set).
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

/**
 * @brief Entry point: parse, resolve, open the socket, then send and
 *        receive until -w runs out or Ctrl-C, and print the statistics.
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
