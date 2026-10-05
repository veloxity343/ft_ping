#include "ft_ping.h"

/**
 * @brief The single global process context (see t_ping in ft_ping.h).
 * Zero-initialised by the C runtime; init_defaults() fills in the
 * non-zero defaults before anything else touches it.
 */
t_ping	g_ping;

/**
 * @brief Zeroes g_ping and fills in the option defaults that aren't 0
 * (TTL, linger, packet size), plus the PID and starting sequence number.
 *
 * Must run before parse_args(), since parse_args() only overwrites the
 * fields the user actually passed flags for.
 */
static void	init_defaults(void)
{
	ft_memset(&g_ping, 0, sizeof(g_ping));
	g_ping.opts.ttl = DEFAULT_TTL;
	g_ping.opts.linger = DEFAULT_TIMEOUT;
	g_ping.opts.packet_size = PACKET_SIZE;
	g_ping.pid = getpid();
	g_ping.seq = 1;
	g_ping.stop = 0;
}

/**
 * @brief Paces the regular (non-preload, non-flood) loop to one send per
 * DEFAULT_INTERVAL second.
 *
 * g_ping.seq counts every send including the preload burst, so preload is
 * subtracted back out of the target-time calculation here - otherwise the
 * burst's sequence numbers would read as if that many intervals had
 * already elapsed, stalling the next send for however many seconds the
 * preload count implies. Also enforces the -w deadline: sets g_ping.stop
 * rather than sleeping past it.
 *
 * @param start_time Wall-clock time the run started, for computing both
 *                    elapsed time and the -w deadline.
 */
static void	sleep_until_next_send(struct timeval *start_time)
{
	struct timeval	now;
	double			elapsed_ms;
	double			target_ms;
	double			wait_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(start_time, &now);
	target_ms = (double)(g_ping.seq - 1 - g_ping.opts.preload)
		* (double)DEFAULT_INTERVAL * 1000.0;
	if (g_ping.opts.timeout > 0
		&& elapsed_ms >= (double)g_ping.opts.timeout * 1000.0)
	{
		g_ping.stop = 1;
		return ;
	}
	if (elapsed_ms >= target_ms)
		return ;
	wait_ms = target_ms - elapsed_ms;
	if (g_ping.opts.timeout > 0)
	{
		if ((double)g_ping.opts.timeout * 1000.0 - elapsed_ms < wait_ms)
			wait_ms = (double)g_ping.opts.timeout * 1000.0 - elapsed_ms;
	}
	if (wait_ms > 0.0)
		usleep((useconds_t)(wait_ms * 1000.0));
	else
		g_ping.stop = 1;
}

/**
 * @brief -f/--flood pacing: caps the send rate at 100/sec (one every
 * FLOOD_POLL_MS), measured from just before this iteration's send.
 *
 * wait_for_icmp_packet() already bounds the reply wait to the same
 * window, so on a fast/lossless link (replies back in well under 10ms)
 * this is what actually enforces the 100 pps ceiling; on a slower or
 * lossy link elapsed_ms is already >= FLOOD_POLL_MS by the time we get
 * here, so no extra sleep is added and sends track reply arrival
 * instead - matching canonical's documented "as fast as they come back
 * or 100 times a second, whichever is more".
 *
 * @param send_time Wall-clock time just before this iteration's
 *                   send_ping() call.
 */
static void	sleep_flood_interval(struct timeval *send_time)
{
	struct timeval	now;
	double			elapsed_ms;

	gettimeofday(&now, NULL);
	elapsed_ms = timeval_diff_ms(send_time, &now);
	if (elapsed_ms < (double)FLOOD_POLL_MS)
		usleep((useconds_t)(((double)FLOOD_POLL_MS - elapsed_ms) * 1000.0));
}

/**
 * @brief -l/--preload: fires `preload` echo requests back-to-back with no
 * pacing, then drains that many replies before falling into the normal
 * one-send-per-iteration loop.
 *
 * Unlike canonical's interleaved async loop, each receive_ping() call
 * here can block waiting on one specific reply; without a -w deadline to
 * bound it, that would let a lossy target stall the drain for up to
 * `preload * linger` seconds. So when the user hasn't set -w, the whole
 * drain phase borrows the deadline machinery (g_ping.opts.timeout) for
 * one shared linger-second window instead of one per packet, then
 * restores the original (absent) deadline afterwards so it doesn't leak
 * into the main loop below.
 *
 * A no-op when preload is 0 (the default, i.e. -l was not given).
 */
static void	send_preload(void)
{
	int	i;
	int	saved_timeout;

	i = 0;
	while (i < g_ping.opts.preload)
	{
		send_ping();
		i++;
	}
	saved_timeout = g_ping.opts.timeout;
	if (g_ping.opts.timeout <= 0)
		g_ping.opts.timeout = g_ping.opts.linger;
	i = 0;
	while (i < g_ping.opts.preload && !g_ping.stop)
	{
		receive_ping();
		i++;
	}
	g_ping.opts.timeout = saved_timeout;
}

/**
 * @brief Entry point.
 *
 * Order of operations: set defaults, parse arguments, resolve the
 * target, open the raw socket (applying every socket-level option:
 * TTL, TOS, SO_RCVTIMEO, SO_DONTROUTE, IP_OPTIONS), install the SIGINT
 * handler, print the banner, fire the preload burst if any, then loop
 * send/receive/pace (flood or regular cadence) until either the -w
 * deadline is hit or SIGINT sets g_ping.stop, and finally print the
 * summary statistics. first_send tracks whether the very next send is
 * the loop's first regular (non-preload) one, so send_ping()'s flood
 * dot can be suppressed for it - canonical's own first "priming" send
 * never gets a dot either.
 *
 * @param argc Argument count, forwarded to parse_args().
 * @param argv Argument vector, forwarded to parse_args().
 * @return Always 0; fatal errors exit() directly from deeper in the
 *         call chain (resolve_target(), open_socket(), print_usage()).
 */
int	main(int argc, char **argv)
{
	struct timeval	now;
	struct timeval	send_time;
	int				first_send;

	init_defaults();
	parse_args(argc, argv);
	resolve_target(g_ping.target_raw);
	open_socket();
	signal(SIGINT, sigint_handler);
	print_start_banner();
	gettimeofday(&g_ping.start_time, NULL);
	send_preload();
	first_send = 1;
	while (!g_ping.stop)
	{
		if (g_ping.opts.timeout > 0)
		{
			gettimeofday(&now, NULL);
			if (timeval_diff_ms(&g_ping.start_time, &now)
					>= (double)g_ping.opts.timeout * 1000.0)
				break ;
		}
		gettimeofday(&send_time, NULL);
		if (send_ping() == 0 && g_ping.opts.flood && !first_send)
			ft_printf(".");
		first_send = 0;
		receive_ping();
		if (!g_ping.stop && g_ping.opts.flood)
			sleep_flood_interval(&send_time);
		else if (!g_ping.stop)
			sleep_until_next_send(&g_ping.start_time);
	}
	print_statistics();
	return (0);
}
