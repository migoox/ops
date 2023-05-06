C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

prog7: prog7.o
	gcc -o prog7 prog7.o ${L_FLAGS}

prog7.o: prog7.c
	gcc -o prog7.o -c prog7.c ${C_FLAGS}