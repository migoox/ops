# c compiler you want to use
CC=gcc
# compiling flags
C_FLAGS=-Wall -fsanitize=address,undefined -pedantic -Os  
# linking flags
L_FLAGS=-Wall -fsanitize=address,undefined
# executable file name
TARGET=my_program
FILES=main.o hello.o

all: ${TARGET}

${TARGET}: ${FILES}
	${CC} -o ${TARGET} ${FILES} ${L_FLAGS}

main.o: main.c hello.h
	${CC} -o main.o -c main.c ${C_FLAGS}

hello.o: hello.c hello.h
	${CC} -o hello.o -c hello.c ${C_FLAGS}

# .PHONY prevents situations when there is a file called "clean" and instead of running
# this command, information about the "clean" file will be shown
.PHONY: clean run

# '-' sign prevent from showing errors 
clean:
	-rm -f ${FILES} ${TARGET}