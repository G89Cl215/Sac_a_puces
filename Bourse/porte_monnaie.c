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
** 
**------------------------------------------------
*/


uint8_t cla, ins, p1, p2, p3;
uint8_t sw1, sw2;
int		taille;
t_ampon		tampon		EEMEM;
t_etat		etat		EEMEM;
char		nom[MAX_PERSO]	EEMEM;
uint16_t	ee_n		EEMEM;
uint8_t		name_size	EEMEM;
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
** Procedure d'engagement d'ecriture dans l'eeprom
*/

void	ft_engager(int n, ...)
{
	va_list		ap;
	uint8_t		*source;
	uint8_t		*dest;
	uint8_t		i;
	uint8_t		tete_buff;

	i = 0;
	etat = 0; //eeprom
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
	etat = plein; //eeprom
}

void	ft_valider(void)
{
	if (etat == plein) //eeprom
	{
		eeprom_read_block();
		eeprom_write_block();
	}
	etat = vide; //eeprom
}

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
	for (i = 0; i < p3; i++)
		eeprom_write_byte(&(nom[i]), data[i]);
	eeprom_write_byte(&name_size, p3);
	sw1 = 0x90;
}

void	get_user_name(void)
{
	int i;

	if (p3 != name_size)
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
	sendbytet0(eeprom_read_byte((uint8_t*)(&ee_n));
	sendbytet0(eeprom_read_byte((uint8_t*)(&ee_n + 1));
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
	sendbytet0(ins); //???
	new_credit = (uint16_t)(recbytet0()) << 8;
	new_credit |= (uint16_t)(recbytet());
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n)) << 8;
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n + 1));
	if (old_credit + new_credit < old_credit
	|| old_credit + new_credit < old_credit)
		sw1 = 0x6c; //ici : mauvais sw, il faut sw de capacite depassee
	else
	{
		new_credit += old_credit;
		eeprom_write_byte((uint8_t*)(&ee_n), (uint8_t)(new_credit >> 8));
		eeprom_write_byte((uint8_t*)(&een) + 1, (uint8_t)(new_credit & 255));
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
	debit |= (uint16_t)(recbytet0()) << 8;
	debit |= (uint16_t)(recbytet0());
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n)) << 8;
	old_credit |= (uint16_t)eeprom_read_byte((uint8_t*)(&ee_n + 1));
	if (debit > old_credit)
		sw1 = 0x6c; //ici : mauvais sw il faut sw de capacite depassee
	else
	{
		debit -= old_credit;
		eeprom_write_byte((uint8_t*)(&ee_n), (uint8_t)(debit >> 8));
		eeprom_write_byte((uint8_t*)(&een) + 1, (uint8_t)(debit & 255));
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

