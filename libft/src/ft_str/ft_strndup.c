/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strndup.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 12:53:41 by rcheong           #+#    #+#             */
/*   Updated: 2024/10/21 13:25:36 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strndup(const char *s, size_t n)
{
	char	*result;
	size_t	len;

	len = ft_strnlen(s, n);
	result = (char *)malloc(len + 1);
	if (!result)
		return (0);
	result[len] = '\0';
	return ((char *)ft_memcpy(result, s, len));
}
