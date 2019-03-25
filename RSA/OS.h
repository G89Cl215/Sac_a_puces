#ifndef OS_H
# define OS_H

# define MOD_SIZE	64
# define MAXI		128
# define PLEIN		43605

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
	uint8_t		buffer[MOD_SIZE + 1];
}		t_ampon;

void	sendbytet0(uint8_t b);
uint8_t	recbytet0(void);

#endif
