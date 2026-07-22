/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcheong <rcheong@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/30 11:53:36 by ryan99            #+#    #+#             */
/*   Updated: 2024/04/30 11:55:07 by rcheong          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PRINTF_H
# define FT_PRINTF_H

# include "libft.h"
# include <stdarg.h>

/* ---------- FLAGS ---------- */

typedef struct s_flags
{
	int	spec;
	int	left;
	int	zero;
	int	precision;
	int	width;
	int	star;
	int	hash;
	int	space;
	int	plus;
}		t_flags;

t_flags	ft_flags_init(void);
t_flags	ft_flag_left(t_flags flags);
t_flags	ft_flag_digit(char c, t_flags flags);
t_flags	ft_flag_width(va_list args, t_flags flags);
int		ft_flag_precision(const char *str, int i, va_list args, t_flags *flags);
int		ft_pad_width(int width, int size, int zero);

/* ---------- SPECS ---------- */

// c
int		ft_print_char(int c, t_flags flags);
int		ft_putchar(int c);
// s
int		ft_print_str(const char *str, t_flags flags);
int		ft_pad_str(const char *str, t_flags flags);
int		ft_str_precision(const char *str, int precision);
int		ft_print_safe_str(const char *str);
// p
int		ft_print_ptr(unsigned long int n, t_flags flags);
int		ft_print_pvalue(unsigned long int n);
void	ft_print_addr(unsigned long int n);
// d, i
int		ft_print_int(int n, t_flags flags);
int		ft_pad_int(char *strint, int n, t_flags flags);
int		ft_print_sign_flag(int n, t_flags *flags);
int		ft_print_intval(char *strint, int n, t_flags flags);
// u
int		ft_print_unsigned(unsigned n, t_flags flags);
int		ft_pad_unsigned(char *strint, t_flags flags);
int		ft_unsigned_str(char *strint, t_flags flags);
// x, X
int		ft_print_hex(unsigned int n, int is_upper, t_flags flags);
int		ft_print_xformat(char *strint, int n, int is_upper, t_flags flags);
int		ft_print_xvalue(char *strint, int n, int is_upper, t_flags flags);
int		ft_print_xprefix(int is_upper);

/* ---------- HELPERS ---------- */

int		ft_ismod(int c);
int		ft_isflag(int c);
int		ft_isspec(int c);
int		ft_ptrhex_len(unsigned long int n);
char	*ft_itoa_long(long n);
char	*ft_printf_xtoa(unsigned long int n, int is_upper);

/* ---------- PRINTF ---------- */

int		ft_printf(const char *format, ...);
int		ft_parse(char *str, va_list args);
int		ft_parse_flags(const char *str, int i, va_list args, t_flags *flags);
int		ft_format(char type, va_list args, t_flags flags);

#endif