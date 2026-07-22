/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_itoa.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 13:04:38 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/04 15:49:13 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static int	digit_count(int n)
{
	int	len;

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

char	*ft_itoa(int n)
{
	int				len;
	char			*s;
	unsigned int	nbr;

	len = digit_count(n);
	s = (char *)malloc(sizeof(char) * len + 1);
	if (!s)
		return (0);
	s[len] = 0;
	if (!n)
		s[0] = '0';
	if (n < 0)
		nbr = -n;
	else
		nbr = n;
	while (nbr)
	{
		s[--len] = nbr % 10 + '0';
		nbr /= 10;
	}
	if (n < 0)
		s[--len] = '-';
	return (s);
}
