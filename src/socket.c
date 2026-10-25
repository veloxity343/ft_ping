#include "ft_ping.h"

/**
 * @brief Prints why socket() failed (with a hint about root) and exits.
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

/**
 * @brief Prints a setsockopt() failure, closes fd and exits.
 */
static void	sockopt_error(int fd, const char *name)
{
	ft_printf("%s: setsockopt(%s): %s\n", PROG_NAME, name, strerror(errno));
	close(fd);
	exit(1);
}

/**
 * @brief Sets IP_TTL from --ttl.
 */
static void	apply_ttl(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TTL,
			&g_ping.opts.ttl, sizeof(g_ping.opts.ttl)) < 0)
		sockopt_error(fd, "IP_TTL");
}

/**
 * @brief Sets IP_TOS from -T.
 */
static void	apply_tos(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TOS,
			&g_ping.opts.tos, sizeof(g_ping.opts.tos)) < 0)
		sockopt_error(fd, "IP_TOS");
}

/**
 * @brief Sets the receive timeout from -W.
 */
static void	apply_timeout(int fd)
{
	struct timeval	timeout;
 
	timeout.tv_sec = g_ping.opts.linger;
	timeout.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
			&timeout, sizeof(timeout)) < 0)
		sockopt_error(fd, "SO_RCVTIMEO");
}

/**
 * @brief Sets SO_DONTROUTE when -r is given.
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

/**
 * @brief Asks the kernel to add an IP timestamp option (tsonly or tsaddr)
 *        to outgoing packets.
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

/**
 * @brief Opens the raw ICMP socket, applies the socket options and stores
 *        it in g_ping.sockfd.
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
