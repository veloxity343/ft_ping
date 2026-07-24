#ifndef FT_PING_H
# define FT_PING_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <signal.h>
# include <unistd.h>
# include <time.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include "libft.h"
# include "ft_printf.h"

// Constants
# define PROG_NAME		"ft_ping"
# define PACKET_SIZE	64
# define MAX_PACKET		4096
# define DEFAULT_TTL	64
# define DEFAULT_TIMEOUT	1
# define DEFAULT_INTERVAL	1

// Command-line options
typedef struct s_opts
{
	int				verbose;       // -v
	int				flood;         // -f
	int				numeric;       // -n
	int				quiet;
	int				ttl;           // --ttl
	int				count;         // -c style / -l preload
	int				pattern_set;   // -p
	unsigned char	pattern[16];
	int				pattern_len;
	int				record_route;  // -R / -r
	int				packet_size;   // -s
	int				timeout;       // -w
	int				linger;        // -W
	int				ip_timestamp;  // --ip-timestamp
}	t_opts;

/*
** Running stats, used to build the summary line on exit
*/
typedef struct s_stats
{
	int		transmitted;
	int		received;
	double	rtt_min;
	double	rtt_max;
	double	rtt_sum;
	double	rtt_sum2;
}	t_stats;

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
	volatile sig_atomic_t	stop;
	struct timeval			start_time;
}	t_ping;

extern t_ping	g_ping;

// parsing.c
int		parse_args(int argc, char **argv);
void	print_usage(int exit_code);

// resolve.c
int		resolve_target(const char *target);

// socket.c
int		open_socket(void);

#endif
