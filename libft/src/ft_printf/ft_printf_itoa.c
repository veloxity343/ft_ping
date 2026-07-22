/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_itoa_printf.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 12:49:04 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 13:56:24 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static size_t	digit_count(long n)
{
	size_t	len;

	len = 0;
	if (n <= 0)
	{
		len++;
		n = -n;
	}
	while (n)
	{
		n /= 10;
		len++;
	}
	return (len);
}

char	*ft_itoa_long(long n)
{
	size_t			len;
	char			*s;
	unsigned long	nbr;

	len = digit_count(n);
	s = ft_calloc(len + 1, sizeof(char));
	if (!s)
		return (NULL);
	s[len] = 0;
	if (!n)
		s[0] = '0';
	if (n < 0)
		nbr = -n;
	else
		nbr = n;
	while (nbr)
	{
		s[--len] = (nbr % 10) + '0';
		nbr /= 10;
	}
	if (n < 0)
		s[--len] = '-';
	return (s);
}
