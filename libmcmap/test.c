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
	int x,z;
	
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
		nbt_print_ascii(stdout,c->raw,6,8);
		//nbt_file_write("/tmp/temp-decoded.nbt",c->raw,NBT_COMPRESS_NONE);
		fprintf(stdout,"\n");
		
		fprintf(stdout,"HeightMap:\n");
		for (z=0;z<4;z++)
			{
			for (x=0;x<16;x++)
				fprintf(stdout,"%02d ",c->light->height[z][x]);
			fprintf(stdout,"\n");
			}
		fprintf(stdout,"SkyLight:\n");
		for (z=0;z<4;z++)
			{
			for (x=0;x<16;x++)
				fprintf(stdout,"%02x ",c->light->sky[64][z][x]);
			fprintf(stdout,"\n");
			}
		fprintf(stdout,"BlockLight:\n");
		for (z=0;z<4;z++)
			{
			for (x=0;x<16;x++)
				fprintf(stdout,"%02x ",c->light->block[64][z][x]);
			fprintf(stdout,"\n");
			}
		fprintf(stdout,"Blocks:\n");
		for (z=0;z<4;z++)
			{
			for (x=0;x<16;x++)
				fprintf(stdout,"%02x ",c->geom->blocks[64][z][x]);
			fprintf(stdout,"\n");
			}
		fprintf(stdout,"Block Data:\n");
		for (z=0;z<4;z++)
			{
			for (x=0;x<16;x++)
				fprintf(stdout,"%02x ",c->geom->data[64][z][x]);
			fprintf(stdout,"\n");
			}
		
		mcmap_region_free(r);
		mcmap_chunk_free(c);
		}
	
	return 0;
	}
