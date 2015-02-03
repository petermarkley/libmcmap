CFLAGS=-g -Wall
DFLAGS=-D __MCMAP_DEBUG
LFLAGS=-lz -lm -lnbt

all:
	@echo Please use either \'make osx\' or \'make linux\'

osx: libmcmap.dylib test shownbt
linux: libmcmap.so test shownbt

clean:
	rm -rf *.so *.dylib *.o test test.dSYM shownbt shownbt.dSYM

test: test.c
	cc -o test test.c libmcmap.* $(CFLAGS) $(LFLAGS)

shownbt: shownbt.c
	cc -o shownbt shownbt.c libmcmap.* $(CFLAGS) $(LFLAGS)

libmcmap.dylib: mcmap.o memdb.o
	cc -dynamiclib -o libmcmap.dylib mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

libmcmap.so: mcmap.o memdb.o
	cc -shared -o libmcmap.so mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

mcmap.o: mcmap.c mcmap.h
	cc -fPIC -c mcmap.c $(CFLAGS)

memdb.o: memdb.c memdb.h
	cc -fPIC -c memdb.c $(CFLAGS)
