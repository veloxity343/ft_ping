#include "ft_ping.h"

/**
 * @brief Attempts to interpret the target as a literal IPv4 address.
 *
 * On success populates g_ping.dest_addr, ip_str and hostname directly
 * from the literal (no resolution needed - ip_str and hostname end up
 * identical).
 *
 * @param target The raw target string, e.g. "127.0.0.1".
 * @return 1 if target parsed as a valid IPv4 address, else 0.
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
 * @brief Resolves the target as a hostname or FQDN via getaddrinfo(),
 * restricted to IPv4.
 *
 * The original hostname string is kept in g_ping.hostname (for the PING
 * banner) alongside the resolved numeric address in g_ping.ip_str - the
 * mandatory requirement to handle FQDN targets is satisfied by this
 * forward lookup; no reverse lookup is ever performed on a reply's
 * source address (see print_reply(), output.c).
 *
 * @param target The raw target string, e.g. "example.com".
 * @return 1 if target resolved to at least one IPv4 address, else 0.
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
 * @brief Resolves the command-line target into g_ping.dest_addr, ip_str
 * and hostname.
 *
 * Tries a direct IPv4 literal first (try_ipv4_lit()), falling back to
 * hostname/FQDN resolution (resolve_hostname()) only if that fails.
 *
 * @param target The raw target string from argv (g_ping.target_raw).
 * @return Always 0 on success. On failure prints an error and exit(2)s -
 *         never returns in that case.
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
 * @brief Reverse-resolves an IPv4 address to a hostname, for the
 * --ip-timestamp tsaddr reply display.
 *
 * Best-effort only: on any failure (unresolvable address, DNS timeout,
 * malformed input) buf is left untouched and 0 is returned, so the caller
 * falls back to printing the numeric address. -n/--numeric suppresses the
 * lookup entirely, matching canonical ping's own use of the flag - this is
 * the only place in this implementation where -n has a visible effect,
 * since the mandatory reply-line format never does reverse DNS regardless
 * of -n.
 *
 * @param addr   Address to resolve, network byte order (as read directly
 *               off the wire - do not host-order convert before calling).
 * @param buf    Destination buffer for the resolved hostname on success.
 * @param buflen Size of buf.
 * @return 1 if buf now holds a resolved hostname, 0 otherwise.
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
