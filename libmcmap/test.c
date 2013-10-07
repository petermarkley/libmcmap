#include <stdio.h>
#include <time.h>
#include <math.h>
#include "./mcmap.h"
#include "./libnbt/nbt.h"

#define INP_MAP "./test_map/saves/New World/"
#define MAX_STR 1024

int main(int argc, char **argv)
	{
	struct mcmap_level *l;
	int x,y,z;
	
	//load
	if ((l = mcmap_level_read(INP_MAP,MCMAP_PARTIAL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	if (mcmap_prime_circle(l,&(l->overworld),0,0,10.0,MCMAP_FULL,1,0) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	//print
	fprintf(stderr,"\nbefore:\n");
	for (z=-16;z<16;z++)
		{
		for (x=-16;x<16;x++)
			fprintf(stderr,"%02d ",mcmap_get_heightmap(&(l->overworld),x,z));
		fprintf(stderr,"\n");
		}
	
	//project bore-dom! :D
	for (y=66;y>=0;y--)
		{
		for (z=-16;z<16;z++)
			{
			for (x=-16;x<16;x++)
				{
				if (sqrt(pow((double)x,2)+pow((double)z,2)) <= 8.0)
					{
					if (y == 60)
						mcmap_set_block(&(l->overworld),x,y,z,MCMAP_GLASS);
					else
						mcmap_set_block(&(l->overworld),x,y,z,MCMAP_AIR);
					}
				}
			}
		}
	
	//save
	if (mcmap_level_write(l,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	//print again
	fprintf(stderr,"\nafter:\n");
	for (z=-16;z<16;z++)
		{
		for (x=-16;x<16;x++)
			fprintf(stderr,"%02d ",mcmap_get_heightmap(&(l->overworld),x,z));
		fprintf(stderr,"\n");
		}
	
	//free
	mcmap_level_free(l);
	return 0;
	}
