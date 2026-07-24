#include "ft_ping.h"

/*
** Raw ICMP sockets needs CAP_NET_RAW. EPERM/EACCES are two errno values
** that indicate privilege problem rather than system fault
*/
static void	handle_socket_error(void)
{
	if (errno == EPERM || errno == EACCES)
		ft_printf("%s: socket: Operation not permitted "
			"(are you root?)\n", PROG_NAME);
	else
		ft_printf("%s: socket: %s\n", PROG_NAME, strerror(errno));
	exit(1);
}

/*
** Opens raw ICMP socket for sending echo requests &
** receiving replies; applies current TTL setting.
** Stores descriptor in g_ping.sockfd.
*/
int	open_socket(void)
{
	int	fd;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd < 0)
		handle_socket_error();
	if (setsockopt(fd, IPPROTO_IP, IP_TTL,
			&g_ping.opts.ttl, sizeof(g_ping.opts.ttl)) < 0)
	{
		ft_printf("%s: setsockopt(IP_TTL): %s\n",
			PROG_NAME, strerror(errno));
		close(fd);
		exit(1);
	}
	g_ping.sockfd = fd;
	return (fd);
}
