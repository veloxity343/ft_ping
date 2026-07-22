/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_flags.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 11:30:14 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 11:30:16 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

t_flags	ft_flags_init(void)
{
	t_flags	flags;

	flags.spec = 0;
	flags.left = 0;
	flags.zero = 0;
	flags.precision = -1;
	flags.width = 0;
	flags.star = 0;
	flags.hash = 0;
	flags.space = 0;
	flags.plus = 0;
	return (flags);
}

t_flags	ft_flag_left(t_flags flags)
{
	flags.left = 1;
	flags.zero = 0;
	return (flags);
}

t_flags	ft_flag_digit(char c, t_flags flags)
{
	if (flags.star == 1)
		flags.width = 0;
	flags.width = (flags.width * 10) + (c - '0');
	return (flags);
}

t_flags	ft_flag_width(va_list args, t_flags flags)
{
	flags.star = 1;
	flags.width = va_arg(args, int);
	if (flags.width < 0)
	{
		flags.left = 1;
		flags.width *= -1;
	}
	return (flags);
}

int	ft_flag_precision(const char *str, int i, va_list args, t_flags *flags)
{
	int	j;

	j = i + 1;
	if (str[j] == '*')
	{
		flags->precision = va_arg(args, int);
		return (j++);
	}
	flags->precision = 0;
	while (ft_isdigit(str[j]))
	{
		flags->precision = (flags->precision * 10) + (str[j] - '0');
		j++;
	}
	return (j);
}
