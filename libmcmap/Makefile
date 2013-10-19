CFLAGS=-g -Wall
DFLAGS=-D __MCMAP_DEBUG
LFLAGS=-lz -lm -lnbt

all: libmcmap.so

test: libmcmap.so test.c
	cc -o test test.c libmcmap.so $(CFLAGS) $(LFLAGS)

clean:
	rm -f *.so *.o test

libmcmap.so: mcmap.o memdb.o
	cc -shared -o libmcmap.so mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

mcmap.o: mcmap.c mcmap.h
	cc -c mcmap.c $(CFLAGS)

memdb.o: memdb.c memdb.h
	cc -c memdb.c $(CFLAGS)
