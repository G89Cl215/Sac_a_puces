#ifndef PORTE_MONNAIE_H
# define PORTE_MONNAIE_H

#define MAX_PERSO	32
#define MAXI		128

typedef	enum	e_tat
{
	vide,
	plein = 2863311530
}		t_etat;

typedef	struct	s_auvegarde
{
	int	n_op;
	int	n[4];
	uint8_t	*d[4];
	uint8_t	buffer[50];
}		t_tampon;

void	sendbytet0(uint8_t b);
uint8_t	recbytet0(void);

#endif
