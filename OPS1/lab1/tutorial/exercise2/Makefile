# sanitizer should be used for compiler and linker as well
C_FLAGS=-Wall -fsanitize=address,undefined -pedantic -Os
L_FLAGS=-fsanitize=address,undefined

.PHONY: clean all

all: prog2

prog2: prog2.o
	gcc -o prog2 prog2.o ${L_FLAGS}

prog2.o: prog2.c
	gcc -o prog2.o -c prog2.c ${C_FLAGS} 

clean:
	rm -f prog2.o prog2