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

static void	sockopt_error(int fd, const char *name)
{
	ft_printf("%s: setsockopt(%s): %s\n", PROG_NAME, name, strerror(errno));
	close(fd);
	exit(1);
}

static void	apply_ttl(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TTL,
			&g_ping.opts.ttl, sizeof(g_ping.opts.ttl)) < 0)
		sockopt_error(fd, "IP_TTL");
}

/*
** -T/--tos. Always applied, defaulting to 0
*/
static void	apply_tos(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TOS,
			&g_ping.opts.tos, sizeof(g_ping.opts.tos)) < 0)
		sockopt_error(fd, "IP_TOS");
}

static void	apply_timeout(int fd)
{
	struct timeval	timeout;
 
	timeout.tv_sec = g_ping.opts.linger;
	timeout.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
			&timeout, sizeof(timeout)) < 0)
		sockopt_error(fd, "SO_RCVTIMEO");
}

/*
** -r/--ignore-routing. Doesn't actually ignore routing,
** but sets SO_DONTROUTE flag on the socket, which is
** closest equivalent
*/
static void	apply_dontroute(int fd)
{
	int	val;
 
	if (!g_ping.opts.ignore_routing)
		return ;
	val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_DONTROUTE, &val, sizeof(val)) < 0)
		sockopt_error(fd, "SO_DONTROUTE");
}

/*
** --ip-timestamp. Has the kernel prepend an IP Timestamp option
** (RFC 791, option 68) to every outgoing packet on this socket:
** tsonly reserves 9 timestamp-only slots, tsaddr reserves 4
** address+timestamp slots. Slots are filled in by transit routers
** and the destination itself, not by us.
*/
static void	apply_ip_timestamp(int fd)
{
	unsigned char	opt[MAX_IPOPTLEN];
	int				len;

	if (!g_ping.opts.ip_timestamp_set)
		return ;
	ft_memset(opt, 0, sizeof(opt));
	len = MAX_IPOPTLEN;
	if (g_ping.opts.ip_timestamp == 1)
		len -= 4;
	opt[IPOPT_OPTVAL] = IPOPT_TS;
	opt[IPOPT_OLEN] = (unsigned char)len;
	opt[IPOPT_OFFSET] = IPOPT_MINOFF + 1;
	opt[3] = (unsigned char)g_ping.opts.ip_timestamp;
	if (setsockopt(fd, IPPROTO_IP, IP_OPTIONS, opt, len) < 0)
		sockopt_error(fd, "IP_OPTIONS");
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
	apply_ttl(fd);
	apply_tos(fd);
	apply_timeout(fd);
	apply_dontroute(fd);
	apply_ip_timestamp(fd);
	g_ping.sockfd = fd;
	return (fd);
}
