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
	struct nbt_tag *t;
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
	
	//save to a different directory
	if ((l->path = (char *)realloc(l->path,(x = (strlen(OUT_MAP)+1)))) == NULL)
		{
		fprintf(stderr,"realloc() returned NULL");
		return -1;
		}
	strncpy(l->path,OUT_MAP,x);
	if ((t = nbt_child_find(l->meta->firstchild,NBT_STRING,"LevelName")) == NULL)
		{
		fprintf(stderr,"malformed level.dat file\n");
		return -1;
		}
	if ((t->payload.p_string = (char *)realloc(t->payload.p_string,(x = (strlen("New World 2")+1)))) == NULL)
		{
		fprintf(stderr,"realloc() returned NULL");
		return -1;
		}
	strncpy(t->payload.p_string,"New World 2",x);
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
	if (mcmap_level_memcheck(l) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	mcmap_level_free(l);
	return 0;
	}
