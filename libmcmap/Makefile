CFLAGS=-g -Wall
DFLAGS=-D __MCMAP_DEBUG
PPOPTS=-I../libnbt/
LNOPTS=-lz -lm -lnbt -L../libnbt/

all: libmcmap.a
	cc -o test test.c libmcmap.a $(CFLAGS) $(DFLAGS) $(PPOPTS) $(LNOPTS)

clean:
	rm -f *.a *.o test

libmcmap.a: mcmap.o memdb.o
	ar -rcs libmcmap.a mcmap.o memdb.o

mcmap.o: mcmap.c mcmap.h
	cc -c mcmap.c $(CFLAGS) $(DFLAGS) $(PPOPTS)

memdb.o: memdb.c memdb.h
	cc -c memdb.c $(CFLAGS) $(DFLAGS)
