#include "ft_ping.h"

/*
** Attempts to interpret target as a literal IPv4 addr
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

/*
** Resolves target as hostname/FQDN via getaddrinfo, restricted to IPv4
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

/*
** Resolves target_raw into dest_addr, ip_str and hostname. Tries
** direct IPv4 literal first, then falls back to hostname/FQDN
** resolution
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
