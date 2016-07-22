CFLAGS=-g -Wall
DFLAGS=-D __NBT_DEBUG
LFLAGS=-lz
DEP=nbt.c nbt.h
DDEP=memdb.c memdb.h
CC=gcc

all:
	@echo Please use one of \'make osx\', \'make linux\', or \'make mingw\'

osx: libnbt.dylib
linux: libnbt.so
mingw: libnbt.dll

clean:
	rm -f *.so *.dylib *.dll *.o

libnbt.dylib: nbt.o memdb.o
	$(CC) -dynamiclib -o libnbt.dylib nbt.o memdb.o $(CFLAGS) $(LFLAGS)

libnbt.so: nbt.o memdb.o
	$(CC) -shared -o libnbt.so nbt.o memdb.o $(CFLAGS) $(LFLAGS)

libnbt.dll: nbt.o memdb.o
	$(CC) -shared -o libnbt.dll nbt.o memdb.o $(CFLAGS) $(LFLAGS)

nbt.o: $(DEP) $(DDEP)
	$(CC) -fPIC -c nbt.c memdb.c $(CFLAGS)
