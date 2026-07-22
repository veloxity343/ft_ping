/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/26 12:17:27 by rcheong           #+#    #+#             */
/*   Updated: 2024/02/26 13:42:43 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static int	ft_word_count(char const *s, char c)
{
	size_t	n;

	n = 0;
	if (*s && *s != c)
		n++;
	while (*s)
	{
		if (*s == c && *(s + 1) != c && *(s + 1))
			n++;
		s++;
	}
	return (n);
}

static char	*ft_dup_substr(char const *s, unsigned int start, unsigned int end)
{
	char	*str;
	size_t	i;

	str = (char *)malloc(end - start + 2);
	if (!str)
		return (0);
	i = 0;
	while (start <= end)
		str[i++] = s[start++];
	str[i] = 0;
	return (str);
}

char	**ft_split(char const *s, char c)
{
	char			**array;
	unsigned int	start;
	unsigned int	i;
	size_t			n;

	if (!s)
		return (0);
	array = (char **)malloc(sizeof(char *) * (ft_word_count(s, c) + 1));
	if (!array)
		return (NULL);
	i = 0;
	while (s[i] == c && c)
		i++;
	start = i--;
	n = -1;
	while (s[++i])
	{
		if (s[i] != c && (s[i + 1] == c || !(s[i + 1])))
			array[++n] = ft_dup_substr(s, start, i);
		if (s[i] == c && s[i + 1] != c && s[i + 1])
			start = i + 1;
	}
	array[++n] = 0;
	return (array);
}
