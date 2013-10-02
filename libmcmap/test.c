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
	struct mcmap_region *r;
	unsigned int cx,cz, rx,rz;
	struct mcmap_chunk *c;
	//struct nbt_tag *p;
	int x,y,z;
	char dir[MAX_STR];
	
	if ((l = mcmap_level_read(INP_MAP,MCMAP_PARTIAL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	snprintf(dir,MAX_STR,"%s%s",INP_MAP,l->overworld.path);
	
	cx = 31; cz = 31;
	rx = -1; rz = -1;
	if ((r = mcmap_region_read(rx,rz,dir)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw = r;
	if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx] = c;
	cx = 0; cz = 31;
	rx = 0; rz = -1;
	if ((r = mcmap_region_read(rx,rz,dir)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw = r;
	if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx] = c;
	cx = 31; cz = 0;
	rx = -1; rz = 0;
	if ((r = mcmap_region_read(rx,rz,dir)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw = r;
	if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx] = c;
	cx = 0; cz = 0;
	rx = 0; rz = 0;
	if ((r = mcmap_region_read(rx,rz,dir)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw = r;
	if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx] = c;
	
	//r = l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw;
	//c = l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx];
	
	fprintf(stdout,"SkyLight:\n");
	for (z=0;z<16;z++)
		{
		for (x=0;x<16;x++)
			fprintf(stdout,"%02x ",(c->light!=NULL?c->light->sky[64][z][x]:0x00));
		fprintf(stdout,"\n");
		}
	
	//mcmap_set_block(&(l->overworld),11,66,0,MCMAP_LEAVES);
	
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
	
	//fprintf(stdout,"Performing lighting update . . .\n");
	//mcmap_light_update(l,&(l->overworld));
	if (mcmap_level_write(l,1) == -1)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	/*fprintf(stdout,"HeightMap:\n");
	for (z=0;z<4;z++)
		{
		for (x=0;x<16;x++)
			fprintf(stdout,"%02d ",c->light->height[z][x]);
		fprintf(stdout,"\n");
		}*/
	fprintf(stdout,"SkyLight:\n");
	for (z=0;z<16;z++)
		{
		for (x=0;x<16;x++)
			fprintf(stdout,"%02x ",c->light->sky[64][z][x]);
		fprintf(stdout,"\n");
		}/*
	fprintf(stdout,"BlockLight:\n");
	for (z=0;z<16;z++)
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
		}*/
	
	mcmap_level_free(l);
	return 0;
	}
