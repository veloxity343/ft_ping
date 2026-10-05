#ifndef FT_PING_H
# define FT_PING_H

# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <signal.h>
# include <unistd.h>
# include <math.h>
# include <time.h>
# include <poll.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <getopt.h>
# include "libft.h"
# include "ft_printf.h"

/* Constants */
# define PROG_NAME			"ft_ping"
# define PACKET_SIZE		64
# define MAX_PACKET			4096
# define MAX_RECV_PACKET	(MAX_PACKET + 128)
# define DEFAULT_TTL		64
# define DEFAULT_TIMEOUT	1
# define DEFAULT_INTERVAL	1
# define FLOOD_POLL_MS		10
# define MIN_PACKET_SIZE	((int)(sizeof(struct icmphdr) + sizeof(struct timeval)))
# define DUP_BITMAP_BYTES	8192 /* one bit per possible 16-bit ICMP seq */

/* Command-line options */
typedef struct s_opts
{
	int				verbose;          // -v
	int				flood;            // -f (root only; moot, raw socket already needs root)
	int				numeric;          // -n (accepted, no-op: see note below)
	int				ttl;              // --ttl
	int				preload;          // -l
	int				pattern_set;      // -p
	unsigned char	pattern[16];
	int				pattern_len;
	int				ignore_routing;   // -r (SO_DONTROUTE)
	int				packet_size;      // -s
	int				timeout;          // -w (deadline, seconds)
	int				linger;           // -W (per-packet reply wait, seconds)
	int				tos;              // -T
	int				ip_timestamp_set; // --ip-timestamp
	int				ip_timestamp;     // 0 = tsonly, 1 = tsaddr
}	t_opts;

/*
** Help table and fully-formatted flag text
*/
typedef struct s_usage
{
	const char	*option;
	const char	*description;
}	t_usage;

/*
** Running stats, used to build the summary line on exit
*/
typedef struct s_stats
{
	int		transmitted;
	int		received;
	int		duplicates;
	double	rtt_min;
	double	rtt_max;
	double	rtt_sum;
	double	rtt_sum2;
}	t_stats;

/**
 * @brief Decoded IP Timestamp option (RFC 791) pulled from a reply's IP
 * header, produced by parse_ip_timestamp_reply() and consumed by
 * print_ip_timestamp_option().
 *
 * @var present  1 if a timestamp option was found in the reply, else 0;
 *               every other field is only meaningful when this is 1.
 * @var tsaddr   1 if the option is address+timestamp (tsaddr), 0 if
 *               timestamp-only (tsonly).
 * @var count    Number of filled slots, 0-9 for tsonly, 0-4 for tsaddr.
 * @var overflow Count of hosts that had no room left to record a
 *               timestamp (the option's overflow nibble), 0-15.
 * @var addrs    Recorded addresses (tsaddr only), raw network byte order
 *               (ready for inet_ntoa/getnameinfo, no host-order conversion
 *               needed or wanted here).
 * @var times    Recorded timestamps, host byte order: bits 0-30 are
 *               milliseconds since midnight UT (RFC 791) and bit 31
 *               marks a non-standard clock source, one raw value per
 *               filled slot exactly as canonical's ping_cvt_time()
 *               interprets it (see print_ts_value(), output.c) - not
 *               pre-masked or converted here.
 */
typedef struct s_ts_option
{
	int				present;
	int				tsaddr;
	int				count;
	int				overflow;
	unsigned int	addrs[9];
	unsigned int	times[9];
}	t_ts_option;

/*
** Global ping context. Relies on process-wide state so sigint
** handler can still print stats and exit cleanly
*/
typedef struct s_ping
{
	t_opts					opts;
	t_stats					stats;
	int						sockfd;
	char					*target_raw;
	char					hostname[256];
	char					ip_str[INET_ADDRSTRLEN];
	struct sockaddr_in		dest_addr;
	pid_t					pid;
	int						seq;
	unsigned char			dup_seen[DUP_BITMAP_BYTES];
	volatile sig_atomic_t	stop;
	struct timeval			start_time;
}	t_ping;

extern t_ping	g_ping;

/* parsing.c */
int		parse_args(int argc, char **argv);
void	print_usage(int exit_code);

/* resolve.c */
int		resolve_target(const char *target);
int		resolve_reverse(struct in_addr addr, char *buf, size_t buflen);

/* socket.c */
int		open_socket(void);

/* icmp.c */
unsigned short	icmp_checksum(void *buf, int len);
int				send_ping(void);
int				receive_ping(void);

/* output.c */
void	print_start_banner(void);
void	print_reply(int bytes, int seq, int ttl, double rtt, int dup);
void	print_statistics(void);
void	print_icmp_error(const char *from_ip, int seq, int type, int code);
void	print_ip_timestamp_option(const t_ts_option *ts);

/* utils.c */
void	sigint_handler(int signo);
double	timeval_diff_ms(struct timeval *start, struct timeval *end);

#endif
