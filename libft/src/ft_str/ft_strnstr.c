/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strnstr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yyan-bin <yyan-bin@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 12:52:28 by rcheong           #+#    #+#             */
/*   Updated: 2025/04/21 23:43:36 by yyan-bin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t	i;
	size_t	needle_len;

	if (!haystack && !len)
		return (0);
	needle_len = ft_strlen(needle);
	if (!needle_len)
		return ((char *) haystack);
	while (*haystack && len)
	{
		if (*needle == *haystack)
		{
			i = 1;
			while ((len - i) && needle[i]
				&& needle[i] == haystack[i])
				i++;
			if (i == needle_len)
				return ((char *) haystack);
		}
		haystack++;
		len--;
	}
	return (0);
}
