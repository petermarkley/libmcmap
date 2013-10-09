#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "./mcmap.h"
#include "./libnbt/nbt.h"
#ifdef __MCMAP_DEBUG
	#include "./memdb.h"
#endif

#define INP_MAP "./test_map/saves/New World/"
#define OUT_MAP "./test_map/saves/New World 2/"
#define MAX_STR 1024

int main(int argc, char **argv)
	{
	struct mcmap_level *l;
	
	if ((l = mcmap_level_new(0,"My Super Test","flat","19",0,MCMAP_GAME_ADVENTURE,0,1,1,0,1,1,1,1,0,0,1,OUT_MAP)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	if (mcmap_level_write(l,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	nbt_print_ascii(stdout,l->meta,-1,16);
	
	if (mcmap_level_memcheck(l) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	#ifdef __MCMAP_DEBUG
	fprintf(stderr,"=====heap summary: %d bytes across %d allocations=====\n",memdb_heap_size(),memdb_heap_num());
	#endif
	mcmap_level_free(l);
	#ifdef __MCMAP_DEBUG
	fprintf(stderr,"=====after freeing: %d bytes across %d allocations=====\n",memdb_heap_size(),memdb_heap_num());
	#endif
	return 0;
	}
