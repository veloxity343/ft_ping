/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strrchr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 12:51:31 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/04 15:53:50 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strrchr(const char *s, int c)
{
	char	ch;
	char	*ptr;

	ch = c;
	ptr = 0;
	s--;
	while (*++s)
		if (*s == ch)
			ptr = (char *) s;
	if (!c)
		return ((char *) s);
	return (ptr);
}
