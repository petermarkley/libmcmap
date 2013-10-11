#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "./mcmap.h"
#include "./libnbt/nbt.h"
#ifdef __MCMAP_DEBUG
	#include "./memdb.h"
#endif

#define OUT_MAP "./maps/My Super Test/"
#define MAX_STR 1024

int main(int argc, char **argv)
	{
	struct mcmap_level *l;
	int x,y,z;
	
	//initialize map
	if ((l = mcmap_level_new(0,"My Super Test","flat","0",0,MCMAP_GAME_ADVENTURE,0,1,1,0,1,1,1,1,0,0,1,0,64,0,OUT_MAP)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	if (mcmap_prime_circle(l,&(l->overworld),0,0,30.0,MCMAP_PARTIAL,1,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	//populate map
	for (y=63;y>=0;y--)
		{
		for (z=-40;z<40;z++)
			{
			for (x=-40;x<40;x++)
				mcmap_set_block(&(l->overworld),x,y,z,MCMAP_GLASS);
			}
		}
	
	//save map
	if (mcmap_level_write(l,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	//clean up
	if (mcmap_level_memcheck(l) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	#ifdef __MCMAP_DEBUG
	fprintf(stderr,"heap summary: %d bytes across %d allocations\n",memdb_heap_size(),memdb_heap_num());
	#endif
	mcmap_level_free(l);
	#ifdef __MCMAP_DEBUG
	fprintf(stderr,"after freeing: %d bytes across %d allocations\n",memdb_heap_size(),memdb_heap_num());
	#endif
	return 0;
	}
