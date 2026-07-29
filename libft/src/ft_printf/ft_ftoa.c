#include "ft_printf.h"

static long	ft_pow10(int exp)
{
	long	result;

	result = 1;
	while (exp-- > 0)
		result *= 10;
	return (result);
}

void	ft_split_float(double value, int precision, long *int_part,
		long *frac_part)
{
	long	scale;
	double	frac;

	scale = ft_pow10(precision);
	*int_part = (long)value;
	frac = value - (double)*int_part;
	*frac_part = (long)(frac * (double)scale + 0.5);
	if (*frac_part >= scale)
	{
		*frac_part -= scale;
		*int_part += 1;
	}
}

char	*ft_frac_to_str(long frac_part, int precision)
{
	char	*digits;
	char	*padded;
	int		len;
	int		i;

	digits = ft_itoa_long(frac_part);
	if (!digits)
		return (NULL);
	len = ft_strlen(digits);
	padded = malloc(precision + 1);
	if (!padded)
	{
		free(digits);
		return (NULL);
	}
	i = 0;
	while (i < precision - len)
		padded[i++] = '0';
	ft_strlcpy(padded + i, digits, (size_t)(precision - i + 1));
	free(digits);
	return (padded);
}
