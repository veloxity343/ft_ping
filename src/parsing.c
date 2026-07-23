#include "ft_ping.h"

/*
** getopt() cannot distinguish -? flag from its own
** return code since both resolve to '?' so we
** hardcode scan argv for str literal -? token
*/
static int	has_help_flag(int argc, char **argv)
{
	int	i;

	i = 1;
	while (i < argc)
	{
		if (ft_strcmp(argv[i], "-?") == 0)
			return (1);
		i++;
	}
	return (0);
}

static void	handle_unknown_option(int opt)
{
	ft_printf("%s: invalid option -- '%c'\n", PROG_NAME, opt);
	print_usage(1);
}

/*
** Parses options and expects target host
*/
int	parse_args(int argc, char **argv)
{
	int	opt;

	if (has_help_flag(argc, argv))
		print_usage(0);
	opterr = 0;
	while ((opt = getopt(argc, argv, "v")) != -1)
	{
		if (opt == 'v')
			g_ping.opts.verbose = 1;
		else
			handle_unknown_option(optopt);
	}
	if (optind >= argc)
	{
		ft_printf("%s: missing host operand\n", PROG_NAME);
		print_usage(1);
	}
	g_ping.target_raw = argv[optind];
	return (0);
}

/*
** Prints usage info
*/
void	print_usage(int exit_code)
{
	if (exit_code == 0)
	{
		ft_printf("Usage: %s [OPTION...] HOST ...\n", PROG_NAME);
		ft_printf("Send ICMP ECHO_REQUEST packets to network hosts.\n\n");
		ft_printf("  -v, --verbose              verbose output\n");
		ft_printf("  -?, --help                 give this help list\n");
	}
	else
	{
		ft_printf("Usage: %s [OPTION...] HOST ...\n", PROG_NAME);
		ft_printf("Try '%s -?' for more information.\n", PROG_NAME);
	}
	exit(exit_code);
}
