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
	
	if ((f = fopen("./test-parsed_chunk.nbt","w")) == NULL)
		{
		fprintf(stderr,"couldn't write chunk for some reason\n");
		return -1;
		}
	fwrite(reg->chunks[0][0].data,1,reg->chunks[0][0].header->length-1,f);
	fclose(f);
	
	mcmap_free_region(reg);
	return 0;
	}
