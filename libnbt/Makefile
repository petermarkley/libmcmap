CFLAGS=-g -Wall
DFLAGS=-D __NBT_DEBUG
DEP=nbt.c nbt.h
DDEP=memdb.c memdb.h

all: libnbt.a

clean:
	rm -f *.a *.o

libnbt.a: nbt.o memdb.o
	ar -rcs libnbt.a nbt.o memdb.o

nbt.o: $(DEP) $(DDEP)
	cc -c nbt.c memdb.c $(CFLAGS) $(DFLAGS)
