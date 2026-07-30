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
** opens raw socket, sends & receives single echo request
** Continuous send/receive loop with SIGINT handling &
** proper summary output
*/
int	main(int argc, char **argv)
{
	init_defaults();
	parse_args(argc, argv);
	resolve_target(g_ping.target_raw);
	open_socket();
	ft_printf("%s: %s resolves to %s\n", PROG_NAME,
		g_ping.hostname, g_ping.ip_str);
	send_ping();
	receive_ping();
	return (0);
}
