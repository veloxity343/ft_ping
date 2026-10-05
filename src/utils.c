#include "ft_ping.h"

/**
 * @brief SIGINT handler. Only sets g_ping.stop; the main loop notices
 * it, breaks out on its own, and prints the summary statistics -
 * deliberately not done from inside the handler itself, since calling
 * non-async-signal-safe functions (ft_printf's write() calls are fine,
 * but the surrounding stats math and buffering are not guaranteed to
 * be) from a signal handler is unsafe.
 *
 * @param signo Unused - required by the signal() handler signature.
 */
void	sigint_handler(int signo)
{
	(void)signo;
	g_ping.stop = 1;
}

/**
 * @brief Computes the difference between two timestamps in milliseconds.
 *
 * @param start The earlier timestamp.
 * @param end   The later timestamp.
 * @return (end - start) in milliseconds. Negative if end precedes start.
 */
double	timeval_diff_ms(struct timeval *start, struct timeval *end)
{
	double	sec_diff;
	double	usec_diff;

	sec_diff = (double)(end->tv_sec - start->tv_sec) * 1000.0;
	usec_diff = (double)(end->tv_usec - start->tv_usec) / 1000.0;
	return (sec_diff + usec_diff);
}
