/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_unsigned.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 13:12:04 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 15:25:57 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_unsigned_str(char *strint, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.precision >= 0)
		count += ft_pad_width(flags.precision - 1, ft_strlen(strint) - 1, 1);
	count += ft_print_safe_str(strint);
	return (count);
}

int	ft_pad_unsigned(char *strint, t_flags flags)
{
	int	count;

	count = 0;
	if (flags.left == 1)
		count += ft_unsigned_str(strint, flags);
	if (flags.precision >= 0 && (size_t)flags.precision < ft_strlen(strint))
		flags.precision = ft_strlen(strint);
	if (flags.precision >= 0)
	{
		flags.width -= flags.precision;
		count += ft_pad_width(flags.width, 0, 0);
	}
	else
		count += ft_pad_width(flags.width, ft_strlen(strint), flags.zero);
	if (flags.left == 0)
		count += ft_unsigned_str(strint, flags);
	return (count);
}

int	ft_print_unsigned(unsigned n, t_flags flags)
{
	char	*strint;
	long	nb;
	int		count;

	nb = n;
	count = 0;
	if (flags.precision == 0 && n == 0)
	{
		count += ft_pad_width(flags.width, 0, 0);
		return (count);
	}
	strint = ft_itoa_long(nb);
	if (!strint)
		return (0);
	count += ft_pad_unsigned(strint, flags);
	free(strint);
	return (count);
}
