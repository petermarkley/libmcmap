CFLAGS=-g -Wall
DFLAGS=-D __NBT_DEBUG
LFLAGS=-lz
DEP=nbt.c nbt.h
DDEP=memdb.c memdb.h

all:
	@echo Please use either \'make osx\' or \'make linux\'

osx: libnbt.dylib
linux: libnbt.so

clean:
	rm -f *.so *.dylib *.o

libnbt.dylib: nbt.o memdb.o
	cc -dynamiclib -o libnbt.dylib nbt.o memdb.o $(CFLAGS) $(LFLAGS)

libnbt.so: nbt.o memdb.o
	cc -shared -o libnbt.so nbt.o memdb.o $(CFLAGS) $(LFLAGS)

nbt.o: $(DEP) $(DDEP)
	cc -fPIC -c nbt.c memdb.c $(CFLAGS)
