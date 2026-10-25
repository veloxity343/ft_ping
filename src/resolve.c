#include "ft_ping.h"

/**
 * @brief Tries the target as a literal IPv4 address.
 */
static int	try_ipv4_lit(const char *target)
{
	if (inet_pton(AF_INET, target, &g_ping.dest_addr.sin_addr) != 1)
		return (0);
	g_ping.dest_addr.sin_family = AF_INET;
	ft_strlcpy(g_ping.ip_str, target, sizeof(g_ping.ip_str));
	ft_strlcpy(g_ping.hostname, target, sizeof(g_ping.hostname));
	return (1);
}

/**
 * @brief Resolves a hostname or FQDN to IPv4 with getaddrinfo().
 */
static int	resolve_hostname(const char *target)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct sockaddr_in	*addr;

	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	if (getaddrinfo(target, NULL, &hints, &res) != 0 || !res)
		return (0);
	addr = (struct sockaddr_in *)res->ai_addr;
	g_ping.dest_addr.sin_family = AF_INET;
	g_ping.dest_addr.sin_addr = addr->sin_addr;
	inet_ntop(AF_INET, &addr->sin_addr, g_ping.ip_str, sizeof(g_ping.ip_str));
	ft_strlcpy(g_ping.hostname, target, sizeof(g_ping.hostname));
	freeaddrinfo(res);
	return (1);
}

/**
 * @brief Resolves the target into g_ping, or prints 'unknown host' and
 *        exits with 2.
 */
int	resolve_target(const char *target)
{
	if (try_ipv4_lit(target))
		return (0);
	if (resolve_hostname(target))
		return (0);
	ft_printf("%s: unknown host\n", PROG_NAME);
	exit(2);
}

/**
 * @brief Reverse-resolves addr into buf for the tsaddr display. Returns 0
 *        if -n is set or no name is found.
 */
int	resolve_reverse(struct in_addr addr, char *buf, size_t buflen)
{
	struct sockaddr_in	sa;

	if (g_ping.opts.numeric)
		return (0);
	ft_memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr = addr;
	if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), buf, buflen,
			NULL, 0, NI_NAMEREQD) != 0)
		return (0);
	return (1);
}
