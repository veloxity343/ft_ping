/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_ptr.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 11:31:33 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 15:47:15 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_ptrhex_len(unsigned long int n)
{
	int	len;

	len = 0;
	if (n == 0)
		return (1);
	while (n >= 1)
	{
		len++;
		n /= 16;
	}
	return (len);
}

void	ft_print_addr(unsigned long int n)
{
	if (n >= 16)
	{
		ft_print_addr(n / 16);
		ft_print_addr(n % 16);
	}
	else
	{
		if (n < 10)
			ft_putchar(n + '0');
		else if (n >= 10)
			ft_putchar((n - 10) + 'a');
	}
}

int	ft_print_pvalue(unsigned long int n)
{
	int	count;

	count = 0;
	if (n == 0)
	{
		count += ft_print_safe_str("0x0");
		return (count);
	}
	count += ft_print_safe_str("0x");
	ft_print_addr(n);
	count += ft_ptrhex_len(n);
	return (count);
}

int	ft_print_ptr(unsigned long int n, t_flags flags)
{
	int	count;

	count = 0;
	if (n == 0)
		flags.width -= ft_strlen("0x0") - 1;
	else
		flags.width -= 2;
	if (flags.left == 1)
		count += ft_print_pvalue(n);
	count += ft_pad_width(flags.width, ft_ptrhex_len(n), 0);
	if (flags.left == 0)
		count += ft_print_pvalue(n);
	return (count);
}
