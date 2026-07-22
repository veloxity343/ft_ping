/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/04 11:49:01 by rcheong           #+#    #+#             */
/*   Updated: 2024/11/09 10:49:40 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

/* char	*ft_strjoin(char *s1, char *s2)
{
	char		*new_str;
	size_t		i;
	size_t		j;

	if (!s1)
	{
		s1 = (char *)malloc(sizeof(char) * 1);
		s1[0] = '\0';
	}
	new_str = (char *)malloc(ft_strlen(s1) + ft_strlen(s2) + 1);
	if (!new_str)
		return (0);
	i = 0;
	while (s1[i] != '\0')
	{
		new_str[i] = s1[i];
		i++;
	}
	j = 0;
	while (s2[j] != '\0')
		new_str[i++] = s2[j++];
	new_str[i] = '\0';
	free(s1);
	return (new_str);
} */

/*
@brief Joins two strings.
@param s1 The first string.
@param s2 The second string.
@return The joined string.
*/
char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*ret;
	size_t	retsize;

	retsize = ft_strlen(s1) + ft_strlen(s2) + 1;
	ret = malloc(sizeof(char) * retsize);
	if (!ret)
		return (0);
	ret[0] = 0;
	ft_strlcat(ret, s1, retsize);
	ft_strlcat(ret, s2, retsize);
	return (ret);
}

/*
@brief Joins two strings and frees the input strings.
@param s1 The first string.
@param s2 The second string.
@return The joined string.
*/
char	*ft_strjoinf(char *s1, char *s2)
{
	char	*joined;
	size_t	total_length;
	size_t	i;
	size_t	j;

	if (!s1 || !s2)
		return (NULL);
	total_length = ft_strlen(s1) + ft_strlen(s2) + 1;
	joined = ft_calloc(total_length, sizeof(char));
	if (!joined)
		return (NULL);
	i = 0;
	while (s1[i])
	{
		joined[i] = s1[i];
		i++;
	}
	j = 0;
	while (s2[j])
		joined[i++] = s2[j++];
	joined[i] = 0;
	return (free(s1), free(s2), joined);
}
