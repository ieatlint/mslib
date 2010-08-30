all:
	gcc -c llist.c -Wall -g
	gcc -c mslib.c -Wall -g
	gcc -c mstest.c -Wall -g
	gcc -o mstest -Wall mstest.o mslib.o llist.o

clean:
	rm -f *.o mstest
