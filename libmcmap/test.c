#include <stdio.h>
#include "./mcmap.h"

int main(int argc, char **argv)
	{
	FILE *f;
	struct mcmap_region *reg;
	unsigned int x,z;
	z = 0; x = 1;
	
	if ((reg = mcmap_read_region(0,0,"./test_map/saves/Experiment Lab/")) == NULL)
		{
		fprintf(stderr,"couldn't parse region for some reason\n");
		return -1;
		}
	
	fprintf(stderr,"stats for chunk (%u,%u):\n",x,z);
	fprintf(stderr,"\tsector offset: %u (derived from %02x %02x %02x)\n",reg->locations[z][x],reg->header->locations[z][x].offset[0],reg->header->locations[z][x].offset[1],reg->header->locations[z][x].offset[2]);
	fprintf(stderr,"\tsector count: %x\n",reg->header->locations[z][x].sector_count);
	fprintf(stderr,"\tpointer was accordingly set to %x (%u bytes after beginning at %x)\n",(unsigned int)(reg->chunks[z][x].header),(unsigned int)(reg->chunks[z][x].header)-(unsigned int)(reg->header),(unsigned int)(reg->header));
	fprintf(stderr,"\tchunk size in bytes: %u\n",reg->chunks[z][x].size);
	
	if ((f = fopen("./test-parsed_chunk.nbt","w")) == NULL)
		{
		fprintf(stderr,"couldn't write chunk for some reason\n");
		return -1;
		}
	fwrite(reg->chunks[z][x].data,1,reg->chunks[z][x].size,f);
	fclose(f);
	
	mcmap_free_region(reg);
	return 0;
	}
