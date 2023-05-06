CC=gcc
CFLAGS= -std=gnu99 -Wall
LIB_PATH=../library/
OBJS_SERVER= server.o mysocklib.o
OBJS_CLIENT= client.o mysocklib.o
OBJS= server.o client.o mysocklib.o

client: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS_CLIENT) -o client

server: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS_SERVER) -o server

server.o: server.c $(LIB_PATH)mysocklib.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c $(LIB_PATH)mysocklib.h
	$(CC) $(CFLAGS) -c client.c

mysocklib.o: $(LIB_PATH)mysocklib.c $(LIB_PATH)mysocklib.h
	$(CC) $(FLAGS) -c $(LIB_PATH)mysocklib.c

.PHONY: clean cleanobj

cleanobj:
	rm -f $(OBJS)
clean:
	rm -f $(OBJS) server client