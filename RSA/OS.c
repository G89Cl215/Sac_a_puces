#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>
#include "OS.h"

/*
**------------------------------------------------
** Programme "RSA" pour carte à puce
** CE PROGRAMME ADOPTE LA CONVENTION BIG ENDIAN
**------------------------------------------------
*/

uint8_t		cla;
uint8_t		ins;
uint8_t		p1;
uint8_t		p2;
uint8_t		p3;
uint8_t		sw1;
uint8_t		sw2;
int		taille;
t_ampon		tampon			EEMEM;
t_etat		etat			EEMEM;
uint8_t		RSA_MOD[MOD_SIZE + 1]	EEMEM;		
uint8_t		RSA_EXP[3]		EEMEM;		
uint8_t		RSA_KEY[MOD_SIZE + 1]	EEMEM;		
uint8_t		status = NEW;
uint8_t		data[MAXI];
uint8_t		MSG[MOD_SIZE];
uint8_t		rep[MOD_SIZE];

/*
** Procédure qui renvoie l'ATR
*/

void	atr(uint8_t n, char* hist)
{
	sendbytet0(0x3b);
	sendbytet0(n);	
	while (n--)
		sendbytet0(*hist++);
	switch (pintry_nbr)
	{
		case 0xff :
			status = NEW;
			break ;
		case 1, 2, 3 :
			status = LOCK;
			break ;
		case 0xff :
			status = BLOCK;
			break ;
	}
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
** Ensemble des instructions possibles avec la carte
*/

void	ft_intro_param(uint8_t *param)
{
	int		i;
	uint32_t 	key[4];

	if (p3 > MOD_SIZE)
	{
		sw1 = 0x6c;
		sw2 = MOD_SIZE;
		return ;
	}
	sendbytet0(ins);
	for (i = 0; i < p3; i++)
		MSG[i] = recbytet0();
	if (param != MSG)
	{
		ft_engager(1, &p3, param, p3, MSG, param[1], 0);
		ft_valider();
	}
	sw1 = 0x90;

}

void	ft_(void)

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
			case 0x90 :
				switch (ins)
				{
					case 0x00 :
						ft_intro_param(RSA_MOD);
						break ;
					case 0x01 :
						ft_intro_param(RSA_EXP);
						break ;
					case 0x02 :
						ft_intro_param(RSA_KEY);
						break ;
					case 0x03 :
						ft_return_param(RSA_MOD);
						break ;
					case 0x04 ://exposant public
						ft_return_param(RSA_EXP);
						break ;
					case 0x11 : //clair et chiffrement
						ft_intro_param(MSG);
						ft_cypher();
						break ;
					case 0x12 : //intro crypto dechiffrement
						ft_intro_param(MSG);
						ft_decypher();
						break ;
					case 0xc0 : //get rep
						ft_return_msg();
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
