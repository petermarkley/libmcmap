#include <stdio.h>
#include "./mcmap.h"

#define INP_MAP "./test_map/saves/Experiment Lab/"
#define OUT_FIL "./test-parsed_chunk.nbt"

int main(int argc, char **argv)
	{
	FILE *f;
	struct mcmap_region *r;
	unsigned int x,z;
	
	if ((r = mcmap_region_read(0,0,INP_MAP)) == NULL)
		{
		fprintf(stderr,"couldn't parse \'%s\' for some reason\n",INP_MAP);
		return -1;
		}
	
	//print stats for each chunk in the region
	for (z=0;z<32;z++)
		{
		for (x=0;x<32;x++)
			{
			if (r->chunks[z][x].header != NULL)
				{
				fprintf(stdout,"stats for chunk (%u,%u):\n",x,z);
				//fprintf(stdout,"\tsector offset: %u (derived from %02x %02x %02x)\n",r->locations[z][x],r->header->locations[z][x].offset[0],r->header->locations[z][x].offset[1],r->header->locations[z][x].offset[2]);
				fprintf(stdout,"\tsector count: %x\n",r->header->locations[z][x].sector_count);
				fprintf(stdout,"\tpointer was accordingly set to %p (%u bytes after beginning at %p)\n",r->chunks[z][x].header,(unsigned int)(r->chunks[z][x].header)-(unsigned int)(r->header),r->header);
				fprintf(stdout,"\tchunk size in bytes: %u\n",r->chunks[z][x].size);
				}
			else
				fprintf(stdout,"chunk (%u,%u) does not exist\n",x,z);
			}
		}
	
	//extract a single chunk and save it to a separate '.nbt' file
	z = 0; x = 1;
	if (r->chunks[z][x].header != NULL)
		{
		if ((f = fopen(OUT_FIL,"w")) == NULL)
			{
			fprintf(stderr,"couldn't write \'%s\' for some reason\n",OUT_FIL);
			return -1;
			}
		fwrite(r->chunks[z][x].data,1,r->chunks[z][x].size,f);
		fclose(f);
		fprintf(stderr,"wrote chunk (%u,%u) to \'%s\'\n",x,z,OUT_FIL);
		}
	
	mcmap_region_free(r);
	return 0;
	}
