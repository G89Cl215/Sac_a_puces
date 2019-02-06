/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   porte_monnaie.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tgouedar <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/02/06 16:16:36 by tgouedar          #+#    #+#             */
/*   Updated: 2019/02/06 16:16:39 by tgouedar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PORTE_MONNAIE_H
# define PORTE_MONNAIE_H

#define MAX_PERSO	32
#define MAXI		128
#define PLEIN		43605

typedef	enum	e_tat
{
	vide,
	plein = PLEIN
}		t_etat;

typedef	struct	s_auvegarde
{
	int16_t		n_op;
	int16_t		n[4];
	uint8_t		*d[4];
	uint8_t		buffer[50];
}		t_tampon;

void	sendbytet0(uint8_t b);
uint8_t	recbytet0(void);

#endif
