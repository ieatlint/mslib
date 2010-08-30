DEBUG=-Wall -g

all:
	gcc -c llist.c ${DEBUG}
	gcc -c mslib.c ${DEBUG}
	gcc -c mstest.c ${DEBUG}
	gcc -o mstest ${DEBUG} mstest.o mslib.o llist.o

clean:
	rm -f *.o mstest
