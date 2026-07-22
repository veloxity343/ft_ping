/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_striccmp.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 12:51:53 by rcheong           #+#    #+#             */
/*   Updated: 2024/10/21 13:25:04 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

/*
@brief Function to perform a case-insensitive comparison between two characters
@param s1 Pointer to the first character
@param s2 Pointer to the second character
*/
int	ft_striccmp(const char *s1, const char *s2)
{
	char	c1;
	char	c2;

	while (*s1 && *s2)
	{
		c1 = ft_tolower(*s1);
		c2 = ft_tolower(*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}
	return (ft_tolower(*s1) - ft_tolower(*s2));
}
