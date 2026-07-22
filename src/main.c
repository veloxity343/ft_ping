#include "ft_ping.h"

/*
** Global ping context, zero-initialised
*/

t_ping	g_ping;

static void	init_defaults(void)
{
	memset(&g_ping, 0, sizeof(g_ping));
	g_ping.opts.ttl = DEFAULT_TTL;
	g_ping.opts.timeout = DEFAULT_TIMEOUT;
	g_ping.opts.packet_size = PACKET_SIZE;
	g_ping.pid = getpid();
	g_ping.seq = 0;
	g_ping.stop = 0;
}

/*
** Temporary entrypoint to have something to build
** against. This will be replaced once we can parse args
*/

int	main(int argc, char **argv)
{
	init_defaults();
	if (argc < 2)
	{
		ft_printf("usage: %s <target>\n", PROG_NAME);
		return (1);
	}
	g_ping.target_raw = argv[1];
	ft_printf("%s: target set to %s\n", PROG_NAME, g_ping.target_raw);
	return (0);
}
