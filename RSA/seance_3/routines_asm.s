; Langage assembleur : 2 types de lignes 
; o> [label:] (optionnel, permet de faire des sauts de memoire type boucle, retour anticipe...) .directive [operande] 
; 			.byte (la donnee de l'octet ecrit en memoire)
;			.ascii[2] (la donnnede la chaine de caractere avec [2] ou sans [] '\0' final sous-entendu) 
;			.data (ou dans la memoire)
;			.text (tout ce qui suit est ecrit dans la memoire flash)
;			.global (definit les symboles de ce module,utilisables a lexterieur)
;			.extern (definit les symboles exterieurs au module mais reconnu par celui-ci)
; o> [label:] (optionnel, permet de faire des sauts de memoire type boucle, retour anticipe...) instruction [operande]
;			(page 616 de la doc pour toutes les insructions possibles)
;
; o> [ligne vide]
; o> [; commentaires]

; Gestion de la memoire machine (p 19 de la doc atmega). les 32 premiers registres de la mempoire machine sont
; accessibles directement par des instructions du processeur. ex : ADD	R3, R4  <=> R3 <- R3 + R4
; les registres R2 a R7 sont sense etre sauvegardes puis restaures par tout appel de fonction
; R1 vaut la valeur du 0
; les parametres et valeurs de retour sont traites dans les registres R25 a R8 (au dela, ils sont stockes dans la pile) :
; les params sont stockes alignes sur les registres pairs (les bandes de memoire continues representant les parametres,
;  debutent sur un registre pair) en convention little endian en descendant
; (le premier octet de plus petit poids d'un param est stocke dans R24 puis R23 etc..))

; la pile : Stack pointer (SP) (la tete de pile est stockee dans les registres 5c et 5d) Pour nous, la pile est
; descendante on empile puis on decremente (l'origine de la pile est l'adresse max).
; PUSH	R3 <=> [SP] <- R3 et SP--
; POP	R4 <=> SP++ et R4 <- [SP]

; Registre d'etat (page 11) :
; Un registre special qui contient 8bits [I][T][H][S][V][N][Z][C] ayant des donnees sur l'operation precedente.
; I : bit permettant les interruptions systeme
; T : interdit toute interruption masquable lorsqu’il est mis à 1. Suite à un RESET, T est mis à 1.
; H : demi retenue ??? Half-carry est mis à 1 lors d’une retenue entre les bits 3 et 4 d’une opération arithmétique
; V : overflow = carry ^ retenue sur b7 (= chgt de signe (allumage du bit de signe) ou debordement de la valeur hors du
; registre)
; N : negative bit, bit7 (de signe) du resultat de loperation precedente
; Z : Zero (resultat precedent == 0)
; C : carry (retenue du resultat precedent)



; NB : -x = ~x + 1 (complement a 2)



; "routines_asm.s"
;-----------------

	.text		; indique que ce qui suit est en flash

			; déclaration des symboles utilisés ailleurs
	.global		add32_a, incr_a, get_response_a, atr_a

			; déclaration des symboles de fonction définis ailleurs
	.extern		envoyer, acquitte


; addition 32 bits
; prototype uint32_t add32_a(uint32_t a, uint32_t b)
; a = (r22, r23, r24, r25)
; b = (r18, r19, r20, r21)
; résultat dans (r22, r23, r24, r25)

add32_a :
	add	r22,r18
	adc	r23,r19
	adc	r24,r20
	adc	r25,r21
	ret

; a <- a + b
; a et b étant sur t octets
; int8_t incrx_a(uint8_t*a, uint8_t*b, uint8_t t)
; a = (r24, r25)
; b = (r22, r23)
; t = r20

incr_a :
	tst	r20		; test de la taille
	breq	inc_f		; retour si la taille est nulle "breq : branch if equal"
	movw	r26,r24		; a dans registre X (deplace 2 octets adresse vers r26, depuis 24)
	movw	r30,r22		; b dans registre Z
	mov	r24,r20		; le résultat est la taille
	clc			; mise à zéro de la carry
inc_b :
	ld	r22,X		; charge l'octet de a dans r24
	ld	r23,Z+		; charge l'octet de b dans r25
	adc	r22,r23		; additionne les octets, résultat dans r24
	st	X+,r22		; mémorise la somme dans a
	dec	r20		; décrémente le compteur de taille
	brne	inc_b		; recommence tant que la taille est non nulle
	brcc	inc_f		; si pas de retenue, ne rien faire d'autre
	clr	r23		; prépare le résultat de la retenue
	adc	r23,r23		; ajoute la retenue à r25
	st	X,r23		; mémorise dans a
	add	r24,r23		; incrémente la taille
inc_f :
	ret

; get response
; prototype void get_response_a()
; utilise les variables p3 et data définies ailleurs
; elles n'ont pas besoin d'être déclarées
get_response_a:
	lds	r18,p3		; charge p3 (variable) dans r18
	lds	r19,t_data	; charge t_data dans r19
	cp	r18,r19		; compare p3 et t_data
	breq	gr1		; si égalité, alors continuer en gr1
	ldi	r18,0x6c	; charge 0x6c dans r18
	sts	sw1,r18		; mémorise r18 dans sw1
	sts	sw2,r19		; mémorise r19, c-à-d t_data dans sw2
	ret			; retour
gr1 :
	rcall	acquitte	; appelle la procédure acquitte
				; appelle envoyer(data,p3)
				; data dans (r24,r25)
				; p3 dans r22
	ldi	r24,lo8(data)	; charge dans r24 la partie basse de l'adresse de data
	ldi	r25,hi8(data)	; charge dans r25 la partie haute de l'adresse de data
	lds	r22,p3		; charge p3 dans r22
	rcall	envoyer		; appelle "envoyer(data,p3)"
	ldi	r24,0x90	; charge 0x90 dans r24
	sts	sw1,r24		; stock 0x90 dans sw1
	ret			; retour

; atr_a(char*h, uint8_t t)
; h = (r24 ,r25)
; t = r22

atr_a :
	movw	r26,r24		; mémorise l'adresse h dans le registre x
				; appeler sendbytet0(0x3b)
	push	r26		; sauvegarde de x
	push	r27
	push	r22		; sauvegarde de t
	ldi	r24,0x3b	; charge 0x3b dans r24
	rcall	sendbytet0
	pop	r24		; restauration de t dans r24
	push	r24		; sauvegarde de t
	rcall	sendbytet0	; envoi de t
	pop	r22		; restauration de t
	pop	r27		; restauration de x
	pop	r26
	tst	r22		; teste si t=0
	breq	fin		; si c'est le cas, retour
boucle :
	ld	r24,X+		; charge *h++
	push	r26		; sauvegarde x
	push	r27
	push	r22		; sauvegarde t
	rcall	sendbytet0	; envoi des octets d'historique
	pop	r22		; restauration t
	pop	r27		; restauration x
	pop	r26
	dec	r22		; décrémente le compteur
	brne	boucle		; boucle si t est non nul
fin :
	ret			; sinon, retour
