CC=gcc
CFLAGS= -std=gnu99 -Wall
LIB_PATH=../library/
OBJ_DIR=obj/
OBJS_SERVER= $(OBJ_DIR)server.o $(OBJ_DIR)mysocklib.o
OBJS_CLIENT= $(OBJ_DIR)client.o $(OBJ_DIR)mysocklib.o
OBJS= $(OBJ_DIR)server.o $(OBJ_DIR)client.o $(OBJ_DIR)mysocklib.o

client: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS_CLIENT) -o client
	
server: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS_SERVER) -o server

$(OBJ_DIR)server.o: server.c $(LIB_PATH)mysocklib.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c server.c -o $(OBJ_DIR)server.o

$(OBJ_DIR)client.o: client.c $(LIB_PATH)mysocklib.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c client.c -o $(OBJ_DIR)client.o

$(OBJ_DIR)mysocklib.o: $(LIB_PATH)mysocklib.c $(LIB_PATH)mysocklib.h | $(OBJ_DIR)
	$(CC) $(FLAGS) -c $(LIB_PATH)mysocklib.c -o $(OBJ_DIR)mysocklib.o

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: clean cleanobj

cleanobj:
	rm -rf $(OBJ_DIR)

clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) server client
	rmdir $(OBJ_DIR)
