/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_int.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 11:49:43 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 12:48:36 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_print_intval(char *strint, int n, t_flags flags)
{
	int	count;

	count = 0;
	if (n < 0)
	{
		if (flags.zero == 0 || flags.precision >= 0)
			count += ft_putchar('-');
	}
	else if (flags.plus == 1 && flags.zero == 0)
		count += ft_putchar('+');
	else if (flags.space == 1 && flags.zero == 0)
		count += ft_putchar(' ');
	if (flags.precision >= 0)
		count += ft_pad_width(flags.precision - 1, ft_strlen(strint) - 1, 1);
	count += ft_print_safe_str(strint);
	return (count);
}

int	ft_print_sign_flag(int n, t_flags *flags)
{
	int	count;

	count = 0;
	if (n < 0 && flags->precision == -1)
	{
		count += ft_putchar('-');
		flags->width--;
	}
	else if (flags->plus == 1)
		count += ft_putchar('+');
	else if (flags->space == 1)
	{
		count += ft_putchar(' ');
		flags->width--;
	}
	return (count);
}

int	ft_pad_int(char *strint, int n, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.zero == 1)
		count += ft_print_sign_flag(n, &flags);
	if (flags.left == 1)
		count += ft_print_intval(strint, n, flags);
	if (flags.precision >= 0 && (size_t)flags.precision < ft_strlen(strint))
		flags.precision = ft_strlen(strint);
	if (flags.precision >= 0)
	{
		flags.width -= flags.precision;
		if (n < 0 && flags.left == 0)
			flags.width -= 1;
		count += ft_pad_width(flags.width, 0, 0);
	}
	else
		count += ft_pad_width(flags.width - flags.plus - flags.space,
				ft_strlen(strint), flags.zero);
	if (flags.left == 0)
		count += ft_print_intval(strint, n, flags);
	return (count);
}

int	ft_print_int(int n, t_flags flags)
{
	char	*strint;
	long	nb;
	int		count;

	nb = n;
	count = 0;
	if (nb < 0)
	{
		nb *= -1;
		if (flags.zero == 0)
			flags.width--;
	}
	if (flags.precision == 0 && n == 0)
	{
		count += ft_pad_width(flags.width, 0, 0);
		return (count);
	}
	strint = ft_itoa_long(nb);
	if (!strint)
		return (0);
	count += ft_pad_int(strint, n, flags);
	free(strint);
	return (count);
}
