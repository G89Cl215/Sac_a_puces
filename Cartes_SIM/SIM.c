#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>
#include "SIM.h"

/*
**------------------------------------------------
** Programme "PUK&PIN" pour carte à puce
** CE PROGRAMME ADOPTE LA CONVENTION LITTLE ENDIAN
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
uint8_t		pintry_nbr		EEMEM = 0xff;
uint8_t		puk_code[CODE_SIZE]	EEMEM;
uint8_t		pin_code[CODE_SIZE]	EEMEM = DEFAULT_PIN;
uint8_t		status = NEW;
uint8_t		data[MAXI];

/*
** Procédure qui renvoie l'ATR
*/
a
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

int	code_cmp(uint8_t *a, uint8_t *b)
{
	int		i;
	unsigned int	cmp;

	cmp = 0;
	i = 0;
	while (i < CODE_SIZE - 1)
	{
		cmp |= a[i] ^ b[i];
		i++;
	}
	return ((cmp));
}

/*
** Ensemble des instructions possibles avec la carte
*/

void	verify_chv(void)
{
	uint8_t		i;

	if (status != LOCK)
	{
		sw1 = 0x6d;
		return ;
	}
	if (p3 != CODE_SIZE)
	{
		sw1 = 0x6c;
		sw2 = CODE_SIZE;
		return ;
	}
	sendbytet0(ins);
	i = 0;
	while (i < p3)
		data[i++] = recbytet0();
	if (code_cmp(data, pin_code))
	{
		if (!(i = eeprom_read_byte()))
		{
			status = BLOCK;
			return ;
		}
		else
		{
			i--;
			sw1 = 0x98;
			sw2 = 0x40 + i;
		}
	}
	else
	{
		i = MAX_TRY;
		status = UNLOCK;
		sw1 = 0x90;
	}
	ft_engager(1, &pintry_nbr, &i, 0);
	ft_valider();
}

void	set_user_puk(void)
{
	uint8_t		i;

	if (status != NEW)
	{
		sw1 = 0x6d;
		return ;
	}
	if (p3 != CODE_SIZE)
	{
		sw1 = 0x6c;
		sw2 = CODE_SIZE;
		return ;
	}
	sendbytet0(ins);
	i = 0;
	while (i < p3)
		data[i++] = recbytet0();
	i = MAX_TRY;
	ft_engager(CODE_SIZE, puk_code, data, 1, pintry_nbr, &i, 0);
	ft_valider();
	status = LOCK;
	sw1 = 0x90;
}


void	vnblock_chv(void)
{
	uint8_t		i;

	if (status != BLOCK)
	{
		sw1 = 0x6d;
		return ;
	}
	if (p3 != 2 * CODE_SIZE)
	{
		sw1 = 0x6c;
		sw2 = 2 * CODE_SIZE;
		return ;
	}
	sendbytet0(ins);
	i = 0;
	while (i < p3)
		data[i++] = recbytet0();
	if (code_cmp(data, puk_code))
	{
		sw1 = 0x98;
		sw2 = 0x04;
	}
	else
	{
		i = MAX_TRY;
		ft_engager(CODE_SIZE, pin_code, &(data[CODE_SIZE]), 1, pintry_nbr, &i, 0);
		ft_valider();
		status = LOCK;
		sw1 = 0x90;
	}
}

void	change_chv(void)
{
	uint8_t		i;

	if (status != UNLOCK)
	{
		sw1 = 0x6d;
		return ;
	}
	if (p3 != 2 * CODE_SIZE)
	{
		sw1 = 0x6c;
		sw2 = 2 * CODE_SIZE;
		return ;
	}
	sendbytet0(ins);
	i = 0;
	while (i < p3)
		data[i++] = recbytet0();
	if (code_cmp(data, pin_code))
	{
		if (!(i = eeprom_read_byte()))
		{
			status = BLOCK;
			return ;
		}
		else
		{
			i--;
			sw1 = 0x98;
			sw2 = 0x40 + i;
		}
		ft_engager(1, &pintry_nbr, &i, 0);
		ft_valider();
	}
	else
	{
		i = MAX_TRY;
		ft_engager(CODE_SIZE, pin_code, &(data[CODE_SIZE]), 1, pintry_nbr, &i, 0);
		ft_valider();
		status = UNLOCK;
		sw1 = 0x90;
	}
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
			case 0xa0 :
				switch (ins)
				{
					case 0x20 :
						verify_chv();
						break ;
					case 0x24 :
						change_chv();
						break ;
					case 0x2c :
						vnblock_chv();
						break ;
					case 0x40 :
						set_user_puk();
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
