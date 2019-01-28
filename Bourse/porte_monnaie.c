#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

//------------------------------------------------
// Programme "Porte_Monnaie" pour carte à puce
// 
//------------------------------------------------


void sendbytet0(uint8_t b);
uint8_t recbytet0(void);

uint8_t cla, ins, p1, p2, p3;
uint8_t sw1, sw2;

int taille;
#define MAX_PERSO	32
#define MAXI		128
char		nom[MAX_PERSO]	EEMEM;
uint16_t	ee_n		EEMEM;
uint8_t		name_size	EEMEM;
uint8_t		data[MAXI];

// Procédure qui renvoie l'ATR
void atr(uint8_t n, char* hist)
{
    	sendbytet0(0x3b);
    	sendbytet0(n);	
    	while (n--)
        	sendbytet0(*hist++);
}

void	set_user_name()
{
    	int	i;

    	if (p3 > MAX_PERSO)
	{
	   	sw1 = 0x6c;
        	sw2 = MAX_PERSO;
		return ;
    	}
	sendbytet0(ins);
	for (i=0;i < p3;i++)
	    data[i] = recbytet0();
	for (i = 0; i < p3; i++)
	    eeprom_write_byte(&(nom[i]), data[i]);
	eeprom_write_byte(&name_size, p3);
	sw1 = 0x90;
}

void	get_user_name()
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


// Programme principal
//--------------------
int main(void)
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
	        		break;
		  	case 2 :
//				get_balance();
	        		break ;
		  	case 3 :
//				credit_balance();
	        		break ;
		  	case 4 :
//				get_balance();
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

