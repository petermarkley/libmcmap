CFLAGS=-g -Wall -lz
DFLAGS=-D __NBT_DEBUG
DEP=nbt.c nbt.h
DDEP=memdb.c memdb.h

all: libnbt.so

clean:
	rm -f *.so *.o

libnbt.so: nbt.o memdb.o
	cc -shared -o libnbt.so nbt.o memdb.o $(CFLAGS)

nbt.o: $(DEP) $(DDEP)
	cc -c nbt.c memdb.c $(CFLAGS)
