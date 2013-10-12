CFLAGS=-Wall
DFLAGS=-D __NBT_DEBUG

all: libnbt.a

debug: libnbt.a_d

clean:
	rm *.a *.o

libnbt.a: nbt.o
	ar -rcs libnbt.a nbt.o

libnbt.a_d: nbt.o_d
	ar -rcs libnbt.a nbt.o memdb.o

nbt.o:
	cc -c nbt.c $(CFLAGS)

nbt.o_d:
	cc -c nbt.c memdb.c $(CFLAGS) $(DFLAGS)
