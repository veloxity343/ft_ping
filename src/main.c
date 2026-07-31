#include "ft_ping.h"

/*
** Global ping context, zero-initialised
*/
t_ping	g_ping;

static void	init_defaults(void)
{
	ft_memset(&g_ping, 0, sizeof(g_ping));
	g_ping.opts.ttl = DEFAULT_TTL;
	g_ping.opts.timeout = DEFAULT_TIMEOUT;
	g_ping.opts.packet_size = PACKET_SIZE;
	g_ping.pid = getpid();
	g_ping.seq = 1;
	g_ping.stop = 0;
}

/*
** Entry point: sets defaults, parses arguments, resolves target,
** opens raw socket, installs SIGINT handler, sends one echo
** per request until interrupted, , printing banner, per-reply lines,
** and closing statistics.
*/
int	main(int argc, char **argv)
{
	init_defaults();
	parse_args(argc, argv);
	resolve_target(g_ping.target_raw);
	open_socket();
	signal(SIGINT, sigint_handler);
	print_start_banner();
	gettimeofday(&g_ping.start_time, NULL);
	while (!g_ping.stop)
	{
		send_ping();
		receive_ping();
		if (!g_ping.stop)
			sleep(DEFAULT_INTERVAL);
	}
	print_statistics();
	return (0);
}
