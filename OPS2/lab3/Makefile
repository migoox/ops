CC=gcc
CFLAGS= -std=gnu99 -Wall
OBJS= main.o mysocklib.o

all: program

program: $(OBJS)
    $(CC) $(CFLAGS) $(OBJS) -o program

main.o: main.c mysocklib.h
    $(CC) $(CFLAGS) -c main.c

mysocklib.o: mysocklib.c mysocklib.h
    $(CC) $(CFLAGS) -c mysocklib.c

clean:
    rm -f $(OBJS) program