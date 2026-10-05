#include "ft_ping.h"

/**
 * @brief Reports why socket() failed and exits.
 *
 * EPERM/EACCES specifically mean the process lacks CAP_NET_RAW (not
 * running as root, or missing the capability) - worth a clearer message
 * than a bare strerror(), since it's the single most common way this
 * program fails to even start.
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
 * @brief Reports a failed setsockopt() call, closes the socket, and
 * exits. Shared by every apply_*() function below.
 *
 * @param fd   The socket descriptor to close before exiting.
 * @param name The option name to show in the error, e.g. "IP_TTL".
 */
static void	sockopt_error(int fd, const char *name)
{
	ft_printf("%s: setsockopt(%s): %s\n", PROG_NAME, name, strerror(errno));
	close(fd);
	exit(1);
}

/**
 * @brief --ttl. Sets the IP_TTL sockopt, applied unconditionally
 * (defaults to DEFAULT_TTL if the user never passed --ttl).
 *
 * @param fd The socket to apply the option to.
 */
static void	apply_ttl(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TTL,
			&g_ping.opts.ttl, sizeof(g_ping.opts.ttl)) < 0)
		sockopt_error(fd, "IP_TTL");
}

/**
 * @brief -T/--tos. Sets the IP_TOS sockopt. Always applied, defaulting
 * to 0 if the user never passed -T.
 *
 * @param fd The socket to apply the option to.
 */
static void	apply_tos(int fd)
{
	if (setsockopt(fd, IPPROTO_IP, IP_TOS,
			&g_ping.opts.tos, sizeof(g_ping.opts.tos)) < 0)
		sockopt_error(fd, "IP_TOS");
}

/**
 * @brief -W/--linger. Sets SO_RCVTIMEO to the per-reply wait budget
 * (default DEFAULT_TIMEOUT seconds).
 *
 * In practice recvfrom() in receive_ping() only ever runs after poll()
 * has already reported the socket readable (see wait_for_icmp_packet()),
 * so this rarely does the actual waiting - it's a defensive backstop
 * against recvfrom() blocking unexpectedly, not the primary wait
 * mechanism.
 *
 * @param fd The socket to apply the option to.
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
 * @brief -r/--ignore-routing. Sets SO_DONTROUTE, a no-op unless -r was
 * given.
 *
 * Doesn't actually bypass IP routing tables the way canonical's
 * equivalent flag is documented to; SO_DONTROUTE (send only to hosts on
 * directly attached networks, skip the routing table) is the closest
 * available socket-level equivalent.
 *
 * @param fd The socket to apply the option to.
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
 * @brief --ip-timestamp. Has the kernel prepend an IP Timestamp option
 * (RFC 791, option 68) to every outgoing packet on this socket, a no-op
 * unless --ip-timestamp was given.
 *
 * tsonly reserves 9 timestamp-only slots (4 bytes each); tsaddr reserves
 * 4 address+timestamp slots (8 bytes each). Slots are filled in by
 * transit routers and the destination itself as the packet travels, not
 * by this program - see parse_ip_timestamp_reply() (icmp.c) for how the
 * filled-in reply is read back and print_ip_timestamp_option() (output.c)
 * for how it's displayed.
 *
 * Byte layout was checked against inetutils 2.0 and 2.6 source directly
 * (identical between versions) and confirmed live on the wire via
 * tcpdump -vv.
 *
 * @param fd The socket to apply the option to.
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
 * @brief Opens the raw ICMP socket used for the whole run, applies every
 * per-socket option, and stores the descriptor in g_ping.sockfd.
 *
 * Requires CAP_NET_RAW (root, in practice). Called once, from main(),
 * before the send/receive loop starts.
 *
 * @return The opened socket descriptor (also left in g_ping.sockfd).
 *         On any failure this exits directly from handle_socket_error()
 *         or sockopt_error() and never returns.
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
