#include <stdio.h>
#include <time.h>
#include "./mcmap.h"
#include "./libnbt/nbt.h"

#define INP_MAP "./test_map/saves/New World/"
#define OUT_FIL "./test-parsed_chunk.nbt"

int main(int argc, char **argv)
	{
	struct mcmap_region *r;
	unsigned int cx,cz;
	struct mcmap_chunk *c;
	
	if ((r = mcmap_region_read(0,0,INP_MAP)) == NULL)
		{
		fprintf(stderr,"couldn't parse \'%s\' for some reason\n",INP_MAP);
		return -1;
		}
	
	cz = 0; cx = 0;
	if (r->chunks[cz][cx].header != NULL)
		{
		//extract a single chunk and pass it to libnbt!
		if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_READ_FULL,1)) == NULL)
			{
			fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
			return -1;
			}
		fprintf(stdout,"\nNBT data from chunk (%u,%u), decrompressed from %u bytes and last updated %s\n",cx,cz,(unsigned int)r->chunks[cz][cx].size,ctime(&(r->dates[cz][cx])));
		nbt_print_ascii(stdout,c->raw,3,16);
		fprintf(stdout,"\n");
		
		nbt_file_write("/tmp/temp_encoded1.nbt",c->raw,NBT_COMPRESS_NONE);
		if (mcmap_chunk_write(r,cx,cz,c,1) == -1)
			{
			fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
			return -1;
			}
		nbt_file_write("/tmp/temp_encoded2.nbt",c->raw,NBT_COMPRESS_NONE);
		
		mcmap_region_free(r);
		mcmap_chunk_free(c);
		}
	
	return 0;
	}
