#include "ft_ping.h"

/*
** --ttl and --ip-timestamp have no short-option equivalent, so they
** need values getopt_long can return that don't collide with any
** real character
*/
# define OPT_TTL			1000
# define OPT_IP_TIMESTAMP	1001

static const struct option	long_options[] = {
    {"verbose",			no_argument,		NULL, 'v'},
    {"flood",			no_argument,		NULL, 'f'},
    {"preload",			required_argument,	NULL, 'l'},
    {"numeric",			no_argument,		NULL, 'n'},
    {"timeout",			required_argument,	NULL, 'w'},
    {"linger",			required_argument,	NULL, 'W'},
    {"pattern",			required_argument,	NULL, 'p'},
    {"ignore-routing",	no_argument,		NULL, 'r'},
    {"size",			required_argument,	NULL, 's'},
    {"tos",				required_argument,	NULL, 'T'},
    {"ttl",				required_argument,	NULL, OPT_TTL},
    {"ip-timestamp",	required_argument,	NULL, OPT_IP_TIMESTAMP},
    {NULL, 0, NULL, 0}
};

static const t_usage	g_usage[] = {
    {"-f, --flood", "flood ping"},
    {"-l, --preload=NUMBER",
        "send NUMBER packets before falling into normal mode"},
    {"-n, --numeric", "do not resolve host addresses"},
    {"-p, --pattern=PATTERN",
        "fill ICMP packet with given pattern (hex)"},
    {"-r, --ignore-routing",
        "send directly to a host on an attached network"},
    {"-s, --size=NUMBER", "send NUMBER data octets"},
    {"-T, --tos=NUM", "set type of service (TOS) to NUM"},
    {"    --ttl=N", "specify N as time-to-live"},
    {"-v, --verbose", "verbose output"},
    {"-w, --timeout=N", "stop after N seconds"},
    {"-W, --linger=N", "number of seconds to wait for response"},
    {"    --ip-timestamp=FLAG",
        "IP timestamp of type FLAG (tsonly|tsaddr)"},
    {"-?, --help", "give this help list"},
    {NULL, NULL}
};

/*
** getopt_long() cannot cleanly distinguish a genuine "-?" flag from
** its own "unrecognised option" return code, since both resolve to
** the character '?'. Rather than fight that ambiguity, argv is
** scanned directly first and a literal "-?" token is treated as an
** immediate request for help.
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

/*
** Validates a required_argument string as a plain non-negative
** integer up to `max`, rejecting empty strings, non-digit characters,
** and out-of-range values outright rather than trusting ft_atoi to
** catch malformed input, which it doesn't reliably do.
*/
static int	parse_uint_arg(const char *s, int max, int *out)
{
    long	result;
    int		i;

    if (!s || !s[0])
        return (0);
    result = 0;
    i = 0;
    while (s[i])
    {
        if (!ft_isdigit(s[i]))
            return (0);
        result = result * 10 + (s[i] - '0');
        if (result > max)
            return (0);
        i++;
    }
    *out = (int)result;
    return (1);
}

static int	hex_val(char c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    if (c >= 'A' && c <= 'F')
        return (c - 'A' + 10);
    return (-1);
}

/*
** Parses hex pattern for -p into up to 16 raw bytes
*/
static int	parse_pattern(const char *s)
{
    int	len;
    int	i;
    int	hi;
    int	lo;

    len = ft_strlen(s);
    if (len == 0 || len % 2 != 0 || len > 32)
        return (0);
    i = 0;
    while (i < len)
    {
        hi = hex_val(s[i]);
        lo = hex_val(s[i + 1]);
        if (hi < 0 || lo < 0)
            return (0);
        g_ping.opts.pattern[i / 2] = (unsigned char)((hi << 4) | lo);
        i += 2;
    }
    g_ping.opts.pattern_len = len / 2;
    g_ping.opts.pattern_set = 1;
    return (1);
}

/* place invalid_arg before any callers to avoid forward decls */
static void	invalid_arg(const char *optname, const char *val)
{
    ft_printf("%s: invalid argument '%s' for '%s'\n",
        PROG_NAME, val, optname);
    print_usage(1);
}

/*
** Parses --ip-timestamp's value: "tsonly" or "tsaddr"
*/
static void	parse_ip_timestamp(const char *s)
{
    if (ft_strcmp(s, "tsonly") == 0)
        g_ping.opts.ip_timestamp = 0;
    else if (ft_strcmp(s, "tsaddr") == 0)
        g_ping.opts.ip_timestamp = 1;
    else
        invalid_arg("--ip-timestamp", s);
    g_ping.opts.ip_timestamp_set = 1;
}

/*
** Handles bonus options that takes required arg
*/
static void	parse_valued_opt(int opt, const char *optname, const char *arg)
{
    int	val;

    if (opt == 'l' && parse_uint_arg(arg, 100000, &val))
        g_ping.opts.preload = val;
    else if (opt == 'w' && parse_uint_arg(arg, 100000, &val) && val > 0)
        g_ping.opts.timeout = val;
    else if (opt == 'W' && parse_uint_arg(arg, 100000, &val) && val > 0)
        g_ping.opts.linger = val;
    else if (opt == 's' && parse_uint_arg(arg, MAX_PACKET - 100, &val))
    {
        if (val < MIN_PACKET_SIZE)
            val = MIN_PACKET_SIZE;
        g_ping.opts.packet_size = val;
    }
    else if (opt == 'T' && parse_uint_arg(arg, 255, &val))
        g_ping.opts.tos = val;
    else if (opt == OPT_TTL && parse_uint_arg(arg, 255, &val) && val > 0)
        g_ping.opts.ttl = val;
    else if (opt == 'p')
    {
        if (!parse_pattern(arg))
            invalid_arg(optname, arg);
        return ;
    }
    else if (opt == OPT_IP_TIMESTAMP)
    {
        parse_ip_timestamp(arg);
        return ;
    }
    else
        invalid_arg(optname, arg);
}

/*
** Maps getopt return value back to long-option name
*/
static const char	*opt_display_name(int opt)
{
    if (opt == 'l')
        return ("--preload");
    if (opt == 'w')
        return ("--timeout");
    if (opt == 'W')
        return ("--linger");
    if (opt == 's')
        return ("--size");
    if (opt == 'T')
        return ("--tos");
    if (opt == 'p')
        return ("--pattern");
    if (opt == OPT_TTL)
        return ("--ttl");
    if (opt == OPT_IP_TIMESTAMP)
        return ("--ip-timestamp");
    return ("option");
}

static void	handle_unknown_option(int opt)
{
    if (opt > 0 && opt < 128)
        ft_printf("%s: invalid option -- '%c'\n", PROG_NAME, opt);
    else
        ft_printf("%s: invalid option\n", PROG_NAME);
    print_usage(1);
}

/*
** Parses all options
*/
int	parse_args(int argc, char **argv)
{
    int	opt;

    if (has_help_flag(argc, argv))
        print_usage(0);
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "vfl:nw:W:p:rs:T:",
                long_options, NULL)) != -1)
    {
        if (opt == 'v')
            g_ping.opts.verbose = 1;
        else if (opt == 'f')
            g_ping.opts.flood = 1;
        else if (opt == 'n')
            g_ping.opts.numeric = 1;
        else if (opt == 'r')
            g_ping.opts.ignore_routing = 1;
        else if (opt == 'l' || opt == 'w' || opt == 'W' || opt == 's'
            || opt == 'T' || opt == 'p' || opt == OPT_TTL
            || opt == OPT_IP_TIMESTAMP)
            parse_valued_opt(opt, opt_display_name(opt), optarg);
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
** Widest option string across the table, so every description starts
** in the same column regardless of how long any one entry is.
*/
static int	max_option_len(void)
{
    int	max;
    int	len;
    int	i;

    max = 0;
    i = 0;
    while (g_usage[i].option)
    {
        len = ft_strlen(g_usage[i].option);
        if (len > max)
            max = len;
        i++;
    }
    return (max);
}

/*
** Prints one help row: the option text, padded to `col`, then its
** description.
*/
static void	print_option(const char *option, const char *desc, int col)
{
    int	pad;

    ft_printf("  %s", option);
    pad = col - (int)ft_strlen(option);
    while (pad-- > 0)
        ft_printf(" ");
    ft_printf("%s\n", desc);
}

/*
** Prints usage information and terminates.
*/
void	print_usage(int exit_code)
{
    int	col;
    int	i;

    if (exit_code == 0)
    {
        ft_printf("Usage: %s [OPTION...] HOST ...\n", PROG_NAME);
        ft_printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
        col = max_option_len() + 2;
        i = 0;
        while (g_usage[i].option)
        {
            print_option(g_usage[i].option, g_usage[i].description, col);
            i++;
        }
    }
    else
    {
        ft_printf("Usage: %s [OPTION...] HOST ...\n", PROG_NAME);
        ft_printf("Try '%s -?' for more information.\n", PROG_NAME);
    }
    exit(exit_code);
}
