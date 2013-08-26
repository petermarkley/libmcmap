#include <stdio.h>
#include "./mcmap.h"

int main(int argc, char **argv)
	{
	FILE *f;
	struct mcmap_region *reg;
	
	if ((reg = mcmap_read_region(0,0,"./test_map/saves/Experiment Lab/")) == NULL)
		{
		fprintf(stderr,"couldn't parse region for some reason\n");
		return -1;
		}
	
	fprintf(stderr,"sector offset for chunk 0,0: %d (derived from %02x %02x %02x)\n",reg->locations[0][0],reg->header->locations[0][0].offset[0],reg->header->locations[0][0].offset[1],reg->header->locations[0][0].offset[2]);
	fprintf(stderr,"sector count for chunk 0,0: %x\n",reg->header->locations[0][0].sector_count);
	fprintf(stderr,"pointer was accordingly set to %x (%d bytes after beginning at %x)\n",(unsigned int)(reg->chunks[0][0].header),(unsigned int)(reg->chunks[0][0].header)-(unsigned int)(reg->header),(unsigned int)(reg->header));
	fprintf(stderr,"chunk size in bytes: %u\n",reg->chunks[0][0].size);
	
	if ((f = fopen("./test-parsed_chunk.nbt","w")) == NULL)
		{
		fprintf(stderr,"couldn't write chunk for some reason\n");
		return -1;
		}
	fwrite(reg->chunks[0][0].data,1,reg->chunks[0][0].size,f);
	fclose(f);
	
	mcmap_free_region(reg);
	return 0;
	}
