#include <stdio.h>
#include <time.h>
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
	int x,z;
	//char dir[MAX_STR];
	cx = 0; cz = 0;
	rx = 0; rz = 0;
	
	if ((l = mcmap_level_read(INP_MAP,MCMAP_READ_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	
	/*snprintf(dir,MAX_STR,"%s%s",INP_MAP,l->overworld.path);
	if ((r = mcmap_region_read(rx,rz,dir)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw = r;
	if ((c = mcmap_chunk_read(&(r->chunks[cz][cx]),MCMAP_READ_FULL,1)) == NULL)
		{
		fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
		return -1;
		}
	l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx] = c;*/
	
	r = l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->raw;
	c = l->overworld.regions[rz-l->overworld.start_z][rx-l->overworld.start_x]->chunks[cz][cx];
	
	//p = nbt_child_find(l->meta->firstchild,NBT_STRING,"LevelName");
	//fprintf(stdout,"\nLevel \'%s\':  . . .\n\n",p->payload.p_string);
	//nbt_print_ascii(stdout,l->meta,-1,32);
	//fprintf(stdout,"\n");
	//fprintf(stdout,"NBT data from chunk (%u,%u), decrompressed from %u bytes and last updated %s\n",cx,cz,(unsigned int)r->chunks[cz][cx].size,ctime(&(r->dates[cz][cx])));
	//nbt_print_ascii(stdout,c->raw,3,8);
	//fprintf(stdout,"\n");
	
	fprintf(stdout,"SkyLight:\n");
	for (z=0;z<16;z++)
		{
		for (x=0;x<16;x++)
			fprintf(stdout,"%02x ",c->light->sky[64][z][x]);
		fprintf(stdout,"\n");
		}
	
	//mcmap_set_block(&(l->overworld),11,66,0,MCMAP_LEAVES);
	
	fprintf(stdout,"Performing lighting update . . .\n");
	mcmap_light_update(&(l->overworld),l);
	
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
	for (z=0;z<4;z++)
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
