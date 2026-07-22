/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_hex.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 13:41:18 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 15:21:12 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_print_xprefix(int is_upper)
{
	if (is_upper == 1)
		ft_print_safe_str("0X");
	else
		ft_print_safe_str("0x");
	return (2);
}

int	ft_print_xvalue(char *strint, int n, int is_upper, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.zero == 0 && flags.hash == 1 && n != 0)
		count += ft_print_xprefix(is_upper);
	if (flags.precision >= 0)
		count += ft_pad_width(flags.precision - 1,
				ft_strlen(strint) - 1, 1);
	count += ft_print_safe_str(strint);
	return (count);
}

int	ft_print_xformat(char *strint, int n, int is_upper, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.zero == 1 && flags.hash == 1 && n != 0)
		count += ft_print_xprefix(is_upper);
	if (flags.left == 1)
		count += ft_print_xvalue(strint, n, is_upper, flags);
	if (flags.precision >= 0 && (size_t)flags.precision < ft_strlen(strint))
		flags.precision = ft_strlen(strint);
	if (flags.precision >= 0)
	{
		flags.width -= flags.precision;
		count += ft_pad_width(flags.width, 0, 0);
	}
	else
		count += ft_pad_width(flags.width,
				ft_strlen(strint) + (flags.hash * 2), flags.zero);
	if (flags.left == 0)
		count += ft_print_xvalue(strint, n, is_upper, flags);
	return (count);
}

int	ft_print_hex(unsigned int n, int is_upper, t_flags flags)
{
	char	*strint;
	int		count;

	count = 0;
	if (flags.precision == 0 && n == 0)
	{
		count += ft_pad_width(flags.width, 0, 0);
		return (count);
	}
	strint = ft_printf_xtoa(n, is_upper);
	if (!strint)
		return (0);
	count += ft_print_xformat(strint, n, is_upper, flags);
	free(strint);
	return (count);
}
