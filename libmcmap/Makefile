CMODULES=mcmap.c test.c
DMODULES=memdb.c
CFLAGS=-lz -lm -lnbt -L../libnbt/ -I../libnbt/
DFLAGS=-D __MCMAP_DEBUG -D __NBT_DEBUG
APPLICATION=test

all:
	cc -g -Wall -o $(APPLICATION) $(CMODULES) $(CFLAGS) 

debug:
	cc -g -Wall -o $(APPLICATION) $(DMODULES) $(CMODULES) $(CFLAGS) $(DFLAGS)
