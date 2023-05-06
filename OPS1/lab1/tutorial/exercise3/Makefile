# sanitizer should be used for compiler and linker as well
C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-fsanitize=address,undefined

.PHONY: clean all

all: prog3

prog3: prog3.o
	gcc -o prog3 prog3.o ${L_FLAGS}

prog3.o: prog3.c
	gcc -o prog3.o -c prog3.c ${C_FLAGS} 

clean:
	rm -f prog3.o prog3