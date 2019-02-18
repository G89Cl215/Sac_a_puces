#ifndef PORTE_MONNAIE_H
# define PORTE_MONNAIE_H

# define MAXI		128
# define PLEIN		43605
# define CODE_SIZE	8
# define NEW		0
# define LOCK		1
# define UNLOCK		2
# define BLOCK		3
# define DEFAULT_PIN	{'0', '0', '0', '0', '0', '0', '0', '0', '0'}
# define MAX_TRY	3

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
