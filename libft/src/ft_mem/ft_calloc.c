/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_calloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/23 12:53:30 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/04 15:26:28 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	*ft_calloc(size_t count, size_t size)
{
	void	*ptr;

	if ((size != 0 && count > UINT_MAX / size)
		|| (count != 0 && size > UINT_MAX / count))
		ptr = NULL;
	ptr = (void *)malloc(size * count);
	if (!ptr)
		return (0);
	ft_bzero(ptr, count * size);
	return (ptr);
}
