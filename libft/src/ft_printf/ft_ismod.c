/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ismod.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/08 11:30:22 by rcheong           #+#    #+#             */
/*   Updated: 2024/03/08 15:47:43 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_isspec(int c)
{
	if (c == 'c' || c == 's' || c == 'd' || c == 'i' || c == 'u' || c == 'x'
		|| c == 'X' || c == 'p' || c == '%')
		return (1);
	return (0);
}

int	ft_isflag(int c)
{
	if (c == '-' || c == '0' || c == '.' || c == '*' || c == '#' || c == ' '
		|| c == '+')
		return (1);
	return (0);
}

int	ft_ismod(int c)
{
	return (ft_isspec(c) || ft_isdigit(c) || ft_isflag(c));
}
