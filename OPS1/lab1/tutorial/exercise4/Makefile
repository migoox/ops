C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

prog4: prog4.o
	gcc -o prog4 prog4.o ${L_FLAGS}

prog4.o: prog4.c
	gcc -o prog4.o -c prog4.c ${C_FLAGS}

.PHONY: clean

clean:
	rm -f prog4.o prog4