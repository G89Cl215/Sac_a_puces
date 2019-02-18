/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   porte_monnaie.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tgouedar <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/02/06 15:15:10 by tgouedar          #+#    #+#             */
/*   Updated: 2019/02/06 16:37:53 by tgouedar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>
#include "porte_monnaie.h"

/*
**------------------------------------------------
** Programme "Porte_Monnaie" pour carte à puce
** CE PROGRAMME ADOPTE LA CONVENTION LITTLE ENDIAN
**------------------------------------------------
*/

uint8_t cla, ins, p1, p2, p3;
uint8_t sw1, sw2;
int		taille;
uint32_t	clef[4]			EEMEM;
t_ampon		tampon			EEMEM;
t_etat		etat			EEMEM;
char		nom[MAX_PERSO]		EEMEM;
uint16_t	ee_n			EEMEM;
uint8_t		name_size		EEMEM;
uint8_t		data[MAXI];

/*
** Procédure qui renvoie l'ATR
*/

void	atr(uint8_t n, char* hist)
{
	sendbytet0(0x3b);
	sendbytet0(n);	
	while (n--)
		sendbytet0(*hist++);
}

/*
** Procedures d'engagement et de validation de
** l'ecriture dans l'eeprom
*/

void	ft_engager(int n, ...)
{
	va_list		ap;
	uint8_t		*source;
	uint8_t		*dest;
	uint8_t		i;
	uint8_t		tete_buff;

	i = 0;
	eeprom_update_word(&etat, 0);
	tete_buff = 0;
	va_start(ap, n);
	while (n)
	{
		source = va_arg(ap, void*);
		dest = va_arg(ap, void*);
		eeprom_write_block(&n, &(tampon.n[i]), 4);
		eeprom_write_block(&dest, &(tampon.d[i]), 2);
		eeprom_write_block(source, &(tampon.buffer[tete_buff]), n);
		tete_buff += n;
		i++;
		n = va_arg(ap, int);
	}
	eeprom_write_block(&i, &(tampon.n_op));
	eeprom_update_word(&etat, PLEIN);
}

void	ft_valider(void)
{
	char		buff[50];
	uint8_t		*source;
	uint8_t		**dest;
	uint8_t		i;
	uint8_t		tete_buff;
	int16_t		n_op;
	int16_t		status;
 
	status = eeprom_read_word(&etat);
	i = 0;
	tete_buff = 0;
	if (status == PLEIN)
	{
		eeprom_read_block(buff, tampon.buffer, 50);	
		n_op = eeprom_read_word(&(tampon.n_op));
		while (i < n_op)
		{
			n = eeprom_read_word(&(tampon.n[i]));
			eeprom_read_block(dest, &(tampon.d[i]), 2);
			eeprom_write_block(&(buff[tete_buff]), *dest, n);
			i++;
			tete_buff += n;
		}
	}
	eeprom_update_word(&etat, 0);
}

/*
** Ensmble des fonctions de cryptographie de la carte
*/

void	set_key(void)
{
	int		i;
	uint32_t 	key[4];

	if (p3 != 16)
	{
		sw1 = 0x6c;
		sw2 = 16;
		return ;
	}
	sendbytet0(ins);
	for (i = 0; i < 16; i++)
		data[i] = recbytet0();
	for (i = 0; i < 4: i++)
		key[i] = data[4 * i] | (data[4 * i + 1] << 8)
			| (data[4 * i + 2] << 16) | (data[4 * i + 3] << 24);
	ft_engager(16, data, clef, 0);
	ft_valider();
	sw1 = 0x90;
}

void	get_response(void)
{
	int	i;

	if (p3 != 8)
	{
		sw1 = 0x6c;
		sw2 = 8;
		return ;
	}
	sendbytet0(ins);
	for (i = 0; i < 8; i++)
		sendbytet0(response[i]);
	sw1 = 0x90;
}

void	intro_clair_chiffre(void)
{
	int	i;

	if (p3 != 8)
	{
		sw1 = 0x6c;
		sw2 = 8;
		return ;
	}
	sendbytet0(ins);
	sw1 = 0x90;
}

void	set_crypto(void)
{
	int	i;

	if (p3 != 8)
	{
		sw1 = 0x6c;
		sw2 = 8;
		return ;
	}
	sendbytet0(ins);
	sw1 = 0x90;
}


/*
** Ensemble des instructions possibles avec la carte
*/

void	set_user_name(void)
{
	int	i;

	if (p3 > MAX_PERSO)
	{
		sw1 = 0x6c;
		sw2 = MAX_PERSO;
		return ;
	}
	sendbytet0(ins);
	for (i = 0; i < p3; i++)
		data[i] = recbytet0();
	ft_engager(p3, nom, data, 1, &name_size, &p3, 0);
	ft_valider();
	sw1 = 0x90;
}


void	get_user_name(void)
{
	int i;

	if (p3 != eeprom_read_byte(name_size))
	{
		sw1 = 0x6c;
		sw2 = name_size;
		return ;
	}
	sendbytet0(ins);
	for (i = 0; i < name_size; i++)
		sendbytet0(eeprom_read_byte(nom[i]));
	sw1 = 0x90;
}

void	get_balance(void)
{
	uint8_t		byte;

	if (p3 != 2)
	{
		sw1 = 0x6c;
		sw2 = 0X02;
		return ;
	}
	sendbytet0(ins);
	sendbytet0(eeprom_read_byte((uint8_t*)(&ee_n + 1));
	sendbytet0(eeprom_read_byte((uint8_t*)(&ee_n));
	sw1 = 0x90;
}

void	credit_balance(void)
{
	uint16_t	new_credit;
	uint16_t	old_credit;
	uint8_t		byte;

	if (p3 != 2)
	{
		sw1 = 0x6c;
		sw2 = 0X02;
		return ;
	}
	sendbytet0(ins);
	new_credit = (uint16_t)(recbytet0());
	new_credit |= (uint16_t)(recbytet()) << 8;
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n));
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n + 1)) << 8;
	if (old_credit + new_credit < old_credit
	|| old_credit + new_credit < old_credit)
		sw1 = 0x6c; //ici : mauvais sw, il faut sw de capacite depassee
	else
	{
		new_credit += old_credit;
		ft_engager(2, (uint8_t*)(&ee_n), (uint8_t*)(&new_credit), 0);
		ft_valider();
	}
	sw1 = 0x90;
}

void	debit_balance(void)
{
	uint16_t	debit;
	uint16_t	old_credit;
	uint8_t		byte;

	new_credit = 0;
	if (p3 != 2)
	{
		sw1 = 0x6c;
		sw2 = 0X02;
		return ;
	}
	sendbytet0(ins);
	debit |= (uint16_t)(recbytet0());
	debit |= (uint16_t)(recbytet0()) << 8;
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n));
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n + 1)) << 8;
	if (debit > old_credit)
		sw1 = 0x6c; //ici : mauvais sw il faut sw de capacite depassee
	else
	{
		debit -= old_credit;
		ft_engager(2, (uint8_t*)(&new_credit), (uint8_t*)(&ee_n), 0);
		ft_valider();
	}
	sw1 = 0x90;
}

/*
**Programme principal
**--------------------
*/

int		main(void)
{
	// initialisation des ports
	ACSR = 0x80;
	DDRB = 0xff;
	DDRC = 0xff;
	DDRD = 0;
	PORTB = 0xff;
	PORTC = 0xff;
	PORTD = 0xff;
	ASSR = 1 << EXCLK;
	TCCR2A = 0;
	ASSR |= 1 << AS2;

	atr(11,"Hello scard");
	ft_valider();
	taille = 0;
	sw2 = 0;
	for (;;)
	{
		cla = recbytet0();
		ins = recbytet0();
		p1 = recbytet0();
		p2 = recbytet0();
		p3 = recbytet0();
		sw2 = 0;
		switch (cla)
		{
			case 0x81 :
				switch (ins)
				{
					case 0 :
						set_user_name();
						break ;
					case 1 :
						get_user_name();
						break ;
					case 2 :
						get_balance();
						break ;
					case 3 :
						credit_balance();
						break ;
					case 4 :
						debit_balance();
						break ;
					case 10 :
						set_key();
						break ;
					case 11 :
						into_clair_chiffre();
						break ;
					case 12 :
						set_crypto();
						break ;
					case 0xc0 :
						get_response();
						break ;
					default :
						sw1 = 0x6d; // code erreur ins inconnu
				}
				break ;
			default :
				sw1 = 0x6e; // code erreur classe inconnue
		}
		sendbytet0(sw1); // envoi du status word
		sendbytet0(sw2);
	}
	return 0;
}
