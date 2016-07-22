CFLAGS=-g -Wall
DFLAGS=-D __MCMAP_DEBUG
LFLAGS=-lz -lm -lnbt
CC=gcc

all:
	@echo Please use one of \'make osx\', \'make linux\', or \'make mingw\'

osx: libmcmap.dylib test shownbt
linux: libmcmap.so test shownbt
mingw: libmcmap.dll test.exe shownbt.exe

clean:
	rm -rf *.so *.dylib *.dll *.o *.exe test test.dSYM shownbt shownbt.dSYM

test: test.c
	$(CC) -o test test.c libmcmap.* $(CFLAGS) $(LFLAGS)

test.exe: test.c
	$(CC) -o test.exe test.c libmcmap.dll $(CFLAGS) $(LFLAGS)

shownbt: shownbt.c
	$(CC) -o shownbt shownbt.c libmcmap.* $(CFLAGS) $(LFLAGS)

shownbt.exe: shownbt.c
	$(CC) -o shownbt.exe shownbt.c libmcmap.dll $(CFLAGS) $(LFLAGS)

libmcmap.dylib: mcmap.o memdb.o
	$(CC) -dynamiclib -o libmcmap.dylib mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

libmcmap.so: mcmap.o memdb.o
	$(CC) -shared -o libmcmap.so mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

libmcmap.dll: mcmap.o memdb.o
	$(CC) -shared -o libmcmap.dll mcmap.o memdb.o $(CFLAGS) $(LFLAGS)

mcmap.o: mcmap.c mcmap.h
	$(CC) -fPIC -c mcmap.c $(CFLAGS)

memdb.o: memdb.c memdb.h
	$(CC) -fPIC -c memdb.c $(CFLAGS)
