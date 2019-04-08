#include <avr/io.h>
#include <inttypes.h>


/*---------------------------------
 * carte de test pour Assembleur AVR 
 * P1 = 0 : routine en C
 * P1 = 1 : routine en assembleur dans asm.c
 *
 * addition 32 bits add32
 * 8a 00 P1 00 08 aa aa aa aa bb bb bb bb
 * calcule la somme a+b sur 4 octets, résultat dans "data"
 *
 *
 * incrément de taille variable incr
 * 8a 01 p1 i p3 a1 ... ai b1 ... bi
 * p3 = i + i 
 * calcule la somme a + b, écrite en C, résultat dans "data"
 *
 * get response
 * 8a c0 00 00 p3
 *
 * version en assembleur de get response dans "asm.s"
 * 8a c1 00 00 p3
 *
 */


// déclaration des fonctions écrites en assembleur dans les fichiers asm.s
uint32_t	add32_a(uint32_t a,uint32_t b);     	// renvoie a+b
int8_t		incr_a(uint8_t*r, uint8_t*a, int8_t t);	// réalise r = r+a sur des entiers de t chiffres
void		get_response_a();          		// get_response écrit en assembleur
void		atr_a(char*h, uint8_t t);		// atr

/* fonction en c équivalente
{
	sendbytet0(0x3b);
	sendbytet0(t);
	while(t--)
	{
		sendbytet0(*h++);
	}
}
*/

// fonctions d'entrée/sortie écrites dans io.c
void		sendbytet0(uint8_t b);
uint8_t		recbytet0(void);

// variables globales en static ram
uint8_t		cla;
uint8_t		ins;
uint8_t		p1;
uint8_t		p2;
uint8_t		p3;	// header de commande
uint8_t		sw1;
uint8_t		sw2;	// status word
#define MAX 100

uint8_t		data[MAX];	// tableau des résultats
uint8_t		t_data;		// taille du résultat


// acquittement
void acquitte()
{
    sendbytet0(ins);
}


// reception de "l" octets écrits dans la table "t" 
void recevoir(uint8_t*t,uint8_t l)
{
	uint8_t i;
	for (i=0;i<l;i++)
	{
		*t++=recbytet0();
	}
}

// envoi de "l" octets de la table "t"
void envoyer(uint8_t *t,uint8_t l)
{
	uint8_t		i;

	for (i = 0; i < l; i++)
		sendbytet0(*t++);
}

// a <- a+b, renvoie la taille du résultat
uint8_t		incr(uint8_t*a, uint8_t*b, uint8_t t)
{
	uint8_t c;	// carry
	uint8_t i;
	uint16_t r;
	c=0;
	for (i=0;i<t;i++)
	{
		r = (uint16_t)a[i] + (uint16_t)b[i] + (uint16_t)c;
		a[i] = r;
		c = r >> 8;
	}
	if (c != 0)
	{
		a[i] = c;
		return t + 1;
	}
	return t;
}

// définition des commandes
void add32_apdu()
{
	uint32_t	a;
	uint32_t	b;

	if (p3 != 8)
	{
		sw1 = 0x6c;
		sw2 = 8;
		return;
	}
	acquitte();
	recevoir((uint8_t*)&a,4);
	recevoir((uint8_t*)&b,4);
	switch (p1)
	{
		case 0 :
			*(uint32_t*)data = a + b;
			t_data = 4;
			break;
		case 1 :
			*(uint32_t*)data = add32_a(a,b);
			t_data = 4;
			break;
	}
	sw1 = 0x90;
}

void incr_apdu()
{
	if (p3 > MAX)	// teste si p3 ne dépasse pas MAX
	{
		sw1 = 0x6c;
		sw2 = MAX;
		return;
	}
	if (p2 + p2 != p3)
	{
		sw1=0x6b;
		return;
	}
	acquitte();
	recevoir(data, p2);
	recevoir(data + p2, p2);
	if (p1 == 0)
        	t_data = incr(data, data + p2, p2);
	else
		t_data = incr_a(data, data + p2, p2);
	sw1 = 0x90;
}

void get_response()
{
	if (p3 != t_data)
	{
		sw1 = 0x6c;
		sw2 = t_data;
		return ;
	}
	acquitte();
	envoyer(data, p3);
	sw1 = 0x90;
}



// Programme principal
//--------------------
int main(void)
{
  // initialisation hardware des ports et des registres
  	// initialisation des ports
	ACSR = 0x80;
	PORTB = 0xff;
	DDRB = 0xff;
	DDRC = 0xff;
	DDRD = 0;
	PORTC = 0xff;
	PORTD = 0xff;
	ASSR = (1 << EXCLK) + (1 << AS2);
	PRR = 0x87;


	atr_a("assembleur",10);

	for(;;)
	{
		// lecture d'une commande ISO
		cla = recbytet0();
		ins = recbytet0();
		p1 = recbytet0();
		p2 = recbytet0();
		p3 = recbytet0();
		sw2 = 0;
		switch (cla)
		{
		case 0x8a :
			switch (ins)
			{
			case 0 :
				add32_apdu();
				break;
			case 1 :
				incr_apdu();
				break;
			case 0xc0 :
				get_response();
				break;
			case 0xc1 :
				get_response_a();
				break;
			default :
				sw1 = 0x6d; // instruction inconnue
			}
			break;
		default :
			sw1 = 0x6e; // classe inconnue
		}
		sendbytet0(sw1);
		sendbytet0(sw2);
	}
	return 0;
}

// fin du ficher

