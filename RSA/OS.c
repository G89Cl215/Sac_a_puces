#include <io.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
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
uint8_t		count0_hi;

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
** Ensemble des instructions possibles avec la carte
** Rsa est une operation longue et lourde en calcul : les lecteurs considerent une operation de plus de 500 ms comme une
** erreur de la carte si celle-ci n'a pas donne de signe de vie. (0x60 = octet nul qui signale que la carte travaille).

** Regarder les histoires d'interruptions... R.I.P.


** registres (?Què?) :	tccr0A
			tccr0b
			tcnt0
			timsk0

** TP2 : A ecrire :	Demarre60
			Fin60
			Routine d'interruption

** fonctionner sur l'horloge interne (c'est mieux, 2 x plus vite(8MHz au lieux de 4MHz), mais pas toujours le cas,
** ca depend des cartes; Depend de la valeur du lowfuse (0xD2 = interne, 0xD0 = externe))

** Exemple de fonctionnement de compteur :
** Si on fonctionne sur 8MHz (une occurrence toutes les 125 ns), 1024 occurences donnent 128 µs par incrementations
** du compteur. soit une interruption toutes les 37.768 ms (compteur sur un octet, 255 ticks avant overflow) donc 15
** signaux nuls avant que le lecteur ne coupe la communication.

** etu : 372cycles d'horloge externe

** Une autre solution eut ete de renvoyer 0x60 a toute les boucles de l'exponentiation
** Faire quelque chose de synchrone avec le programme de dechiffrement revele immediatement la clef privee a qui ecoute
** ex : Dans une expponentiation rapide, le temps d'une instruction depend de la position des bits allumes dans la clef
** privee 1 -> une elevation au carre + une multiplication, 0 -> juste la multiplication
*/

ISR(TIMER0_OVF_vect)
{
	count0_hi--;
	if (!(count0_hi))
	{
		sendbytet0(0x60);
		count0_hi = 16;
	}
}

void		ft_demarre_60(void)
{
	count0_hi = 16;
	TCCR0A = 0;
	TIMSK0 = 1;	//valide l'interruption sur overflow du compteur CNT0
	TCNT0 = 0;
	TCCR0B = 5;	//lance le compteur CNT0 avec predivision par 1024
	sei();		//positionne le flag d'interruptions
}

void		ft_fin_60(void)
{
	cli();
	TIMSK0 = 0;
	TCCR0B = 0;
}

void		ft_intro_param(uint8_t *param)
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
		ft_engager(1, &p3, param, p3, MSG, param + 1, 0);
		ft_valider();
	}
	sw1 = 0x90;
}

void		ft_return_param(uint8_t *param)
{
	uint8_t		size;
	uint8_t		i;

	i = 0;
	size = eeprom_read_byte(param);
	if (p3 != size)
	{
		sw1 = 0x6c;
		sw2 = size;
		return ;
	}
	sendbytet0(ins);
	eeprom_read_block(MSG, param + 1, size);
	while (i < size)
		sendbytet0(MSG[i++]);
	sw1 = 0x90;
}

void		ft_return_msg()
{
	uint8_t		size;
	uint8_t		i;

	i = 1;
	if (p3 != rep[0])
	{
		sw1 = 0x6c;
		sw2 = rep[0];
		return ;
	}
	sendbytet0(ins);
	while (i < rep[0] + 1)
		sendbytet0(rep[i++]);
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
						ft_demarre_60();
						ft_intro_param(MSG);
						ft_cypher();
						ft_fin_60();
						break ;
					case 0x12 : //intro crypto dechiffrement
						ft_demarre_60();
						ft_intro_param(MSG);
						ft_decypher();
						ft_fin_60();
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
