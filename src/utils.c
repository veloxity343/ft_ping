#include "ft_ping.h"

/*
** SIGINT handler: only sets a flag.
** Summary and exit handled later in main loop
*/
void	sigint_handler(int signo)
{
	(void)signo;
	g_ping.stop = 1;
}

/*
** Millisecond difference between two timestamps
*/
double	timeval_diff_ms(struct timeval *start, struct timeval *end)
{
	double	sec_diff;
	double	usec_diff;

	sec_diff = (double)(end->tv_sec - start->tv_sec) * 1000.0;
	usec_diff = (double)(end->tv_usec - start->tv_usec) / 1000.0;
	return (sec_diff + usec_diff);
}
