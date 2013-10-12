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
#define PI 3.14159265358979323

int main(int argc, char **argv)
	{
	struct mcmap_level *l;
	int x,y,z, bottom, mid;
	double start_phase, end_phase, width, lrad, max, min;
	
	//initialize map
	if ((l = mcmap_level_new(0,"My Super Test","flat","0",0,MCMAP_GAME_CREATIVE,0,1,1,0,1,1,1,1,0,0,1,0,41,0,OUT_MAP)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	if (mcmap_prime_circle(l,&(l->overworld),0,0,30.0,MCMAP_PARTIAL,1,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	//create floating island
	mid = 40; bottom = 25; width = 20.0;
	start_phase = 90.0; end_phase = 260.0;
	for (y=mid;y>=bottom;y--)
		{
		max = sin(start_phase*PI/180.0);
		min = sin(end_phase*PI/180.0);
		lrad = ((sin(   (( (((double)mid)-((double)y))/((double)(mid-bottom)) ) * (end_phase-start_phase)+start_phase)*PI/180.0   )-min)/(max-min))*width;
		for (z=-40;z<40;z++)
			{
			for (x=-40;x<40;x++)
				{
				if (sqrt(pow(x,2)+pow(z,2)) < lrad)
					mcmap_set_block(&(l->overworld),x,y,z,MCMAP_STONE);
				}
			}
		}
	mid = 44; bottom = 41; width = 20.0;
	start_phase = 00.0; end_phase = 90.0;
	for (y=mid;y>=bottom;y--)
		{
		max = sin(end_phase*PI/180.0);
		min = sin(start_phase*PI/180.0);
		lrad = ((sin(   (sin(( (((double)mid)-((double)y))/((double)(mid-bottom)) )*90.0*PI/180.0) * (end_phase-start_phase)+start_phase)*PI/180.0   )-min)/(max-min))*width;
		for (z=-40;z<40;z++)
			{
			for (x=-40;x<40;x++)
				{
				if (sqrt(pow(x,2)+pow(z,2)) < lrad)
					mcmap_set_block(&(l->overworld),x,y,z,MCMAP_DIRT);
				}
			}
		}
	for (y=mid;y>=bottom;y--)
		{
		for (z=-40;z<40;z++)
			{
			for (x=-40;x<40;x++)
				{
				if (mcmap_get_block(&(l->overworld),x,y,z) == MCMAP_DIRT && (mcmap_get_block(&(l->overworld),x,y+1,z) == MCMAP_AIR || mcmap_get_block(&(l->overworld),x,y+1,z) == MCMAP_TALL_GRASS))
					{
					mcmap_set_block(&(l->overworld),x,y,z,MCMAP_GRASS);
					mcmap_set_block(&(l->overworld),x,y+1,z,MCMAP_TALL_GRASS);
					mcmap_set_data(&(l->overworld),x,y+1,z,0x01);
					}
				}
			}
		}
	for (z=-40;z<40;z++)
		{
		for (x=-40;x<40;x++)
			mcmap_set_biome(&(l->overworld),x,z,MCMAP_PLAINS);
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
