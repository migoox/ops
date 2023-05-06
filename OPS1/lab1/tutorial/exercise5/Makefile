C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

prog5: prog5.o
	gcc -o prog5 prog5.o ${L_FLAGS}

prog5.o: prog5.c
	gcc -o prog5.o -c prog5.c ${C_FLAGS}

.PHONY: clean

clean:
	rm -f prog5.o prog5