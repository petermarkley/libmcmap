#include <stdio.h>
#include <time.h>
#include "./mcmap.h"
#include "./libnbt/nbt.h"

#define INP_MAP "./test_map/saves/Experiment Lab/"
#define OUT_FIL "./test-parsed_chunk.nbt"

int main(int argc, char **argv)
	{
	struct mcmap_region *r;
	unsigned int x,z;
	struct mcmap_chunk *c;
	
	if ((r = mcmap_region_read(0,0,INP_MAP)) == NULL)
		{
		fprintf(stderr,"couldn't parse \'%s\' for some reason\n",INP_MAP);
		return -1;
		}
	
	z = 0; x = 1;
	if (r->chunks[z][x].header != NULL)
		{
		//extract a single chunk and pass it to libnbt!
		c = mcmap_chunk_read(&(r->chunks[z][x]),MCMAP_READ_FULL);
		mcmap_region_free(r);
		fprintf(stdout,"\nNBT data from chunk (%u,%u), decrompressed from %u bytes and last updated %s\n",x,z,(unsigned int)r->chunks[z][x].size,ctime(&(r->dates[z][x])));
		nbt_print_ascii(stdout,c->raw,3,10);
		fprintf(stdout,"\n");
		mcmap_chunk_free(c);
		}
	
	return 0;
	}
