NAME=prog10

C_FLAGS=-Wall -fsanitize=address,undefined -pedantic
L_FLAGS=-Wall -fsanitize=address,undefined

${NAME}: ${NAME}.o
	gcc -o ${NAME} ${NAME}.o ${L_FLAGS}
	
${NAME}.o: ${NAME}.c
	gcc -o ${NAME}.o -c ${NAME}.c ${C_FLAGS}

