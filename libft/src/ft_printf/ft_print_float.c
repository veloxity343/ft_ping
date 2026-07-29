#include "ft_printf.h"

static int	ft_print_float_sign(double value, t_flags *flags)
{
	int	count;

	count = 0;
	if (value < 0)
	{
		count += ft_putchar('-');
		flags->width--;
	}
	else if (flags->plus == 1)
	{
		count += ft_putchar('+');
		flags->width--;
	}
	else if (flags->space == 1)
	{
		count += ft_putchar(' ');
		flags->width--;
	}
	return (count);
}

static int	ft_print_floatval(char *strval, double value, t_flags flags)
{
	int	count;

	count = 0;
	if (value < 0)
	{
		if (flags.zero == 0)
			count += ft_putchar('-');
	}
	else if (flags.plus == 1 && flags.zero == 0)
		count += ft_putchar('+');
	else if (flags.space == 1 && flags.zero == 0)
		count += ft_putchar(' ');
	count += ft_print_safe_str(strval);
	return (count);
}

static int	ft_pad_float(char *strval, double value, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.zero == 1)
		count += ft_print_float_sign(value, &flags);
	if (flags.left == 1)
		count += ft_print_floatval(strval, value, flags);
	count += ft_pad_width(flags.width - flags.plus - flags.space,
			ft_strlen(strval), flags.zero);
	if (flags.left == 0)
		count += ft_print_floatval(strval, value, flags);
	return (count);
}

int	ft_print_float(double value, t_flags flags)
{
	long	int_part;
	long	frac_part;
	char	*int_str;
	char	*frac_str;
	char	strval[128];
	int		count;

	if (flags.precision < 0)
		flags.precision = 6;
	if (value < 0 && flags.zero == 0)
		flags.width--;
	ft_split_float(value < 0 ? -value : value, flags.precision,
		&int_part, &frac_part);
	int_str = ft_itoa_long(int_part);
	if (!int_str)
		return (0);
	ft_strlcpy(strval, int_str, sizeof(strval));
	free(int_str);
	if (flags.precision > 0)
	{
		frac_str = ft_frac_to_str(frac_part, flags.precision);
		if (frac_str)
		{
			ft_strlcat(strval, ".", sizeof(strval));
			ft_strlcat(strval, frac_str, sizeof(strval));
			free(frac_str);
		}
	}
	count = ft_pad_float(strval, value, flags);
	return (count);
}
