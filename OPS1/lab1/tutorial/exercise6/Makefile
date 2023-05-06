C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

prog6: prog6.o
	gcc -o prog6 prog6.o ${L_FLAGS}

prog6.o: prog6.c
	gcc -o prog6.o -c prog6.c ${C_FLAGS}