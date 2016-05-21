CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm -ledit
all : lisp
lisp: lisp.o mpc.o
	$(CC) $(CFLAGS) -o lisp lisp.o mpc.o $(LDFLAGS)
mpc: mpc.o
	gcc -c mpc.c
clean:
	rm *.o
