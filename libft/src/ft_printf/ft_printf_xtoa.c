/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_xtoa_printf.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 13:30:25 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 14:18:00 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static size_t	hex_len(long n)
{
	size_t	len;

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

static char	hex_char(int mod, int is_upper)
{
	if (mod < 10)
		return (mod + '0');
	else
	{
		if (is_upper)
			return ((mod - 10) + 'A');
		else
			return ((mod - 10) + 'a');
	}
}

char	*ft_printf_xtoa(unsigned long int n, int is_upper)
{
	size_t	len;
	char	*s;
	int		mod;

	len = hex_len(n);
	s = (char *)ft_calloc(len + 1, sizeof(char));
	if (!s)
		return (NULL);
	len--;
	while (len != (size_t)-1)
	{
		mod = n % 16;
		s[len] = hex_char(mod, is_upper);
		n /= 16;
		len--;
	}
	return (s);
}
