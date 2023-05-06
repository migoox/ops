C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

prog8: prog8.o
	gcc -o prog8 prog8.o ${L_FLAGS}

prog8.o: prog8.c
	gcc -o prog8.o -c prog8.c ${C_FLAGS}