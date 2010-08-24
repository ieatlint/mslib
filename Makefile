TEST="HI"
LIBRARIES=glib-2.0

CFL=`pkg-config --cflags ${LIBRARIES}`
LIBR=`pkg-config --libs ${LIBRARIES}`

all:
	gcc -c mslib.c -Wall -g ${CFL}
	gcc -c mstest.c -Wall -g ${CFL}
	gcc -o mstest -Wall ${LIBR} mstest.o mslib.o

clean:
	rm -f *.o mstest
