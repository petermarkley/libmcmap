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
	struct nbt_tag *t;
	
	if ((r = mcmap_region_read(0,0,INP_MAP)) == NULL)
		{
		fprintf(stderr,"couldn't parse \'%s\' for some reason\n",INP_MAP);
		return -1;
		}
	
	z = 0; x = 1;
	if (r->chunks[z][x].header != NULL)
		{
		//extract a single chunk and pass it to libnbt!
		t = mcmap_chunk_read(&(r->chunks[z][x]));
		mcmap_region_free(r);
		fprintf(stdout,"\nNBT data from chunk (%u,%u):\n\n",x,z);
		nbt_print_ascii(stdout,t,3,10);
		fprintf(stdout,"\n");
		nbt_free_all(t);
		}
	
	return 0;
	}
