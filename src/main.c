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

static void	sleep_until_next_send(struct timeval *start_time)
{
	struct timeval	now;
	double			elapsed_ms;
	double			target_ms;
	double			wait_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(start_time, &now);
	target_ms = (double)(g_ping.seq - 1) * (double)DEFAULT_INTERVAL * 1000.0;
	if (g_ping.opts.timeout > 0
		&& elapsed_ms >= (double)g_ping.opts.timeout * 1000.0)
	{
		g_ping.stop = 1;
		return ;
	}
	if (elapsed_ms >= target_ms)
	{
		return ;
	}
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
** Entry point: sets defaults, parses arguments, resolves target,
** opens raw socket, installs SIGINT handler, sends one echo
** per request until interrupted, , printing banner, per-reply lines,
** and closing statistics.
*/
int	main(int argc, char **argv)
{
	struct timeval	now;

	init_defaults();
	parse_args(argc, argv);
	resolve_target(g_ping.target_raw);
	open_socket();
	signal(SIGINT, sigint_handler);
	print_start_banner();
	gettimeofday(&g_ping.start_time, NULL);
	while (!g_ping.stop)
	{
		if (g_ping.opts.timeout > 0)
		{
			gettimeofday(&now, NULL);
			if (timeval_diff_ms(&g_ping.start_time, &now)
					>= (double)g_ping.opts.timeout * 1000.0)
				break ;
		}
		send_ping();
		receive_ping();
		if (!g_ping.stop)
			sleep_until_next_send(&g_ping.start_time);
	}
	print_statistics();
	return (0);
}
