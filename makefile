PRINTF_PATH	= ../Printf/
PRINTF_LIB	= $(addprefix $(PRINTF_PATH)/,libftprintf.a)

name = rsa

all		: $(name).o $(PRINTF_LIB)
	gcc -o $(name) $(name).o

$(PRINTF_LIB)	:
	make -C $(PRINTF_PATH)

$(name).o: $(name).c
	gcc -c -Wall $(name).c

clean		:
	rm -f $(name) *.o
	make fclean -C $(PRINTF_PATH)

run		: $(name)
	./$(name)

re		: clean all
