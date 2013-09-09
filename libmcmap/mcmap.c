//---------------------------------------------------------------------
//                         libmcmap
//               Minecraft map reading & writing
//      < http://www.minecraftwiki.net/wiki/Level_Format >
// 
// Written by and Copyright 2013 Peter Markley <quartz@malexmedia.net>
// Distributed under the terms of the GNU General Public License.
// 
// Minecraft is the property of Mojang and a trademark of Notch
// Developement AB. Libmcmap is developed by a third party under the
// terms provided at < http://minecraft.net/terms >.
// 
// This file is part of libmcmap.
// 
// Libmcmap is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Libmcmap is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with libmcmap.  If not, see < http://www.gnu.org/licenses/ >.
// 
//---------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "mcmap.h"
#include "libnbt/nbt.h"
#include "cswap.h"

//searches the given path to a minecraft map folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; if error returns NULL
struct mcmap_region *mcmap_region_read(int ix, int iz, const char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	struct stat r_stat;
	uint8_t *buff;
	struct mcmap_region *r;
	unsigned int x,z,i;
	
	//resolve filename from map directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sregion/r.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/region/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((r_file = fopen(r_name,"r")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on \'%s\': %s",r_name,strerror(errno));
		return NULL;
		}
	//determine filesize...
	if (fstat(fileno(r_file),&r_stat) != 0)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fstat() on \'%s\': %s",r_name,strerror(errno));
		return NULL;
		}
	//allocate buffer...
	if ((buff = (uint8_t *)calloc(r_stat.st_size,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//copy file to buffer...
	if ((i = fread(buff,1,r_stat.st_size,r_file)) != r_stat.st_size)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fread() encountered %s on the last %d requested bytes of \'%s\'",(ferror(r_file)?"an error":"EOF"),(unsigned int)r_stat.st_size-i,r_name);
		return NULL;
		}
	//don't need this anymore...
	fclose(r_file);
	
	//allocate navigation structure
	if ((r = (struct mcmap_region *)calloc(1,sizeof(struct mcmap_region))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	
	//engage structure to memory space
	r->header = (struct mcmap_region_header *)buff;
	for (z=0;z<32;z++)
		{
		for (x=0;x<32;x++)
			{
			if (r->header->locations[z][x].sector_count > 0)
				{
				//extract big-endian 32-bit integer from r->header->dates[z][x]
				r->dates[z][x] = cswapr_32(&(r->header->dates[z][x]));
				//extract big-endian 24-bit integer from r->header->location[z][x].offset
				r->locations[z][x] = cswapr_24(&(r->header->locations[z][x].offset));
				
				//chunk listing should not point anywhere in the file header
				if (r->locations[z][x] < 2)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed with invalid location %u",r_name,x,z,r->locations[z][x]);
					return NULL;
					}
				//chunk listing should not point past the end of the file
				i = r->locations[z][x]*4096;
				if (i+r->header->locations[z][x].sector_count*4096 > r_stat.st_size)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed to inhabit %u 4KiB sectors ending at byte %u; file is only %u bytes long",r_name,x,z,r->header->locations[z][x].sector_count,i+((unsigned int)r->header->locations[z][x].sector_count)*4096,(unsigned int)r_stat.st_size);
					return NULL;
					}
				
				//mark flag
				r->chunks[z][x].separate = 0;
				//connect 5-byte chunk header
				r->chunks[z][x].header = (struct mcmap_region_chunk_header *)&(buff[i]);
				//extract big-endian 32-bit integer from r->chunks[z][x].header->length (same location as buff[i])
				r->chunks[z][x].size = cswapr_32(&(buff[i]));
				//'r->chunks[z][x].data' will now point to a block of 'r->chunks[z][x].size' bytes
				r->chunks[z][x].data = &(buff[i+5]);
				
				//listed chunk size should not be larger than the rest of the file
				if (i+5+r->chunks[z][x].size > r_stat.st_size)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed to be %u bytes when only %u bytes remain of the file",r_name,x,z,(unsigned int)r->chunks[z][x].size,((unsigned int)r_stat.st_size)-(i+5));
					return NULL;
					}
				//in fact neither should it be larger than the sector count in the file header
				if (r->chunks[z][x].size+5 > r->header->locations[z][x].sector_count*4096)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed to be %u bytes, which exceeds the %u bytes designated in the header",r_name,x,z,(unsigned int)r->chunks[z][x].size,r->header->locations[z][x].sector_count*4096);
					return NULL;
					}
				}
			else
				{
				r->chunks[z][x].header = NULL;
				r->chunks[z][x].size = 0;
				r->chunks[z][x].data = NULL;
				}
			}
		}
	
	return r;
	}

//free all memory allocated in 'mcmap_region_read()' or 'mcmap_region_new()'
void mcmap_region_free(struct mcmap_region *r)
	{
	free(r->header); //free the memory buffer of the file contents
	free(r); //free the navigation nodes that were connected throughout the buffer
	return;
	}

//takes an individual chunk from a 'struct mcmap_region,' returns a parsed 'mcmap_chunk;'
//'mode' should be MCMAP_READ_FULL for fully populated chunk, MCMAP_READ_PARTIAL to save memory
//on simple geometry inquiries; 'rem' is a boolean flag for whether to remember the raw NBT structure; returns NULL on error
struct mcmap_chunk *mcmap_chunk_read(struct mcmap_region_chunk *rc, mcmap_readmode mode, int rem)
	{
	nbt_compression_type type;
	struct mcmap_chunk *c;
	struct nbt_tag *t, *p, *l;
	int8_t y_index;
	unsigned int x,y,z, i;
	
	//don't index into NULL struct
	if (rc == NULL || rc->header == NULL)
		return NULL;
	//allocate target struct
	if ((c = (struct mcmap_chunk *)calloc(1,sizeof(struct mcmap_chunk))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//determine compression type
	switch (rc->header->compression)
		{
		case 0x1: type=NBT_COMPRESS_GZIP; break;
		case 0x2: type=NBT_COMPRESS_ZLIB; break;
		default: type=NBT_COMPRESS_UNKNOWN; break;
		}
	//read input
	if ((c->raw = nbt_decode(rc->data,rc->size,type)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	
	//allocate both geometry struct and chunk metadata struct
	if ((c->geom = (struct mcmap_chunk_geom *)calloc(1,sizeof(struct mcmap_chunk_geom))) == NULL || (c->meta = (struct mcmap_chunk_meta *)calloc(1,sizeof(struct mcmap_chunk_meta))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//drill past root container
	if ((t = nbt_child_find(c->raw,NBT_COMPOUND,"Level")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	//read biomes
	if ((p = nbt_child_find(t,NBT_BYTE_ARRAY,"Biomes")) != NULL)
		{
		if (p->payload.p_byte_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		memcpy(c->geom->biomes,p->payload.p_byte_array.data,256);
		}
	//read modified time
	if ((p = nbt_child_find(t,NBT_LONG,"LastUpdate")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	c->meta->mtime = p->payload.p_long;
	//read populated flag
	if ((p = nbt_child_find(t,NBT_BYTE,"TerrainPopulated")) != NULL)
		c->meta->populated = p->payload.p_byte;
	else
		c->meta->populated = 0x00;
	//read inhabited time
	if ((p = nbt_child_find(t,NBT_LONG,"InhabitedTime")) != NULL) //this is supposedly a required tag for a chunk, but older maps (pre-1.6.1) won't have them so we won't sound the alarm
		c->meta->itime = p->payload.p_long;
	//read X & Z coords
	if ((p = nbt_child_find(t,NBT_INT,"xPos")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	c->x = p->payload.p_int;
	if ((p = nbt_child_find(t,NBT_INT,"zPos")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	c->z = p->payload.p_int;
	
	//read optional data
	if (mode == MCMAP_READ_FULL)
		{
		//allocate both lighting info struct and special object struct
		if ((c->light = (struct mcmap_chunk_light *)calloc(1,sizeof(struct mcmap_chunk_light))) == NULL || (c->special = (struct mcmap_chunk_special *)calloc(1,sizeof(struct mcmap_chunk_special))) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return NULL;
			}
		//height map
		if ((p = nbt_child_find(t,NBT_INT_ARRAY,"HeightMap")) == NULL || p->payload.p_int_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		memcpy(c->light->height,p->payload.p_int_array.data,256*4);
		//special objects
		if ((c->special->entities = nbt_child_find(t,NBT_LIST,"Entities")) == NULL || (c->special->tile_entities = nbt_child_find(t,NBT_LIST,"TileEntities")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		c->special->tile_ticks = nbt_child_find(t,NBT_LIST,"TileTicks");
		}
	
	//read sections
	if ((t = nbt_child_find(t,NBT_LIST,"Sections")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	for (l = t->firstchild; l != NULL; l = l->next_sib)
		{
		
		//determine Y index
		if ((p = nbt_child_find(l,NBT_BYTE,"Y")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		//store Y index for a minute
		y_index = p->payload.p_byte;
		//block IDs
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Blocks")) == NULL || p->payload.p_byte_array.size != 4096)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		//no memcpy() this time, our arrays are different types/sizes
		i=0;
		for (y = y_index*16; y < (y_index+1)*16; y++)
			{
			for (z=0;z<16;z++)
				{
				for (x=0;x<16;x++)
					{
					c->geom->blocks[y][z][x] = ((uint16_t)p->payload.p_byte_array.data[i])&0x00FF;
					i++;
					}
				}
			}
		//optional addition to higher 4 bits for every block ID
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Add")) != NULL)
			{
			if (p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			i=0;
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						if (i%2 == 0)
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0xF0))<<4)&0x0F00);
						else
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0x0F))<<8)&0x0F00);
						i++;
						}
					}
				}
			}
		//block metadata
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Data")) == NULL || p->payload.p_byte_array.size != 2048)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		i=0;
		for (y = y_index*16; y < (y_index+1)*16; y++)
			{
			for (z=0;z<16;z++)
				{
				for (x=0;x<16;x++)
					{
					if (i%2 == 0)
						c->geom->data[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
					else
						c->geom->data[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
					i++;
					}
				}
			}
		//optional lighting info
		if (mode == MCMAP_READ_FULL)
			{
			//block-emitted light
			if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"BlockLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			i=0;
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						if (i%2 == 0)
							c->light->block[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
						else
							c->light->block[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						i++;
						}
					}
				}
			//sky-emitted light
			if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"SkyLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			i=0;
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						if (i%2 == 0)
							c->light->sky[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
						else
							c->light->sky[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						i++;
						}
					}
				}
			}
		
		}
	
	if (!rem)
		{
		nbt_free(c->raw);
		c->raw = NULL;
		}
	
	return c;
	}

//update height map based on geometry (unnecessary before calling 'mcmap_chunk_write()'; will be called anyway)
void mcmap_chunk_height_update(struct mcmap_chunk *c)
	{
	int x,y,z;
	if (c != NULL && c->geom != NULL && c->light != NULL)
		{
		for (z=0;z<16;z++)
			{
			for (x=0;x<16;x++)
				{
				for (y=256; y>0 && (c->geom->blocks[z][y-1][x] == MCMAP_AIR || c->geom->blocks[z][y-1][x] == MCMAP_GLASS || c->geom->blocks[z][y-1][x] == MCMAP_GLASS_PANE); y--);
				c->light->height[z][x] = y;
				}
			}
		}
	return;
	}

//save all existing components of the given chunk to raw NBT data; return 0 on success and -1 on failure
int _mcmap_chunk_nbt_save(struct mcmap_chunk *c)
	{
	struct nbt_tag *Level, *xPos, *zPos, *LastUpdate, *TerrainPopulated, *InhabitedTime, *Biomes, *HeightMap, *Sections, *loop, *probe, *Entities, *TileEntities, *TileTicks;
	struct nbt_tag *s[16]; //an array of 16 pointers, each to point at an NBT_COMPOUND in the 'Sections' list
	unsigned int ishere1, ishere2, add; //flags used for various branch logic
	int x,y,z, i,j;
	
	//make sure we have the bare-minimum chunk-conforming NBT structure...
	//find each piece (except 'HeightMap', 'Entities', 'TileEntities', & 'TileTicks' for now);
	//if we can't find it, create it
	if (c->raw == NULL)
		{
		if ((c->raw = nbt_child_new(NULL,NBT_COMPOUND,NULL)) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		ishere1 = 0;
		}
	else
		ishere1 = 1;
	if (!ishere1 || (Level = nbt_child_find(c->raw,NBT_COMPOUND,"Level")) == NULL)
		{
		if ((Level = nbt_child_new(c->raw,NBT_COMPOUND,"Level")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (xPos = nbt_child_find(Level,NBT_INT,"xPos")) == NULL)
		{
		if ((xPos = nbt_child_new(Level,NBT_INT,"xPos")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (zPos = nbt_child_find(Level,NBT_INT,"zPos")) == NULL)
		{
		if ((zPos = nbt_child_new(Level,NBT_INT,"zPos")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (LastUpdate = nbt_child_find(Level,NBT_LONG,"LastUpdate")) == NULL)
		{
		if ((LastUpdate = nbt_child_new(Level,NBT_LONG,"LastUpdate")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (TerrainPopulated = nbt_child_find(Level,NBT_BYTE,"TerrainPopulated")) == NULL)
		{
		if ((TerrainPopulated = nbt_child_new(Level,NBT_BYTE,"TerrainPopulated")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (InhabitedTime = nbt_child_find(Level,NBT_LONG,"InhabitedTime")) == NULL)
		{
		if ((InhabitedTime = nbt_child_new(Level,NBT_LONG,"InhabitedTime")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	if (!ishere1 || (Biomes = nbt_child_find(Level,NBT_BYTE_ARRAY,"Biomes")) == NULL)
		{
		if ((Biomes = nbt_child_new(Level,NBT_BYTE_ARRAY,"Biomes")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		Biomes->payload.p_byte_array.size = 256;
		if ((Biomes->payload.p_byte_array.data = (int8_t *)calloc(Biomes->payload.p_byte_array.size,1)) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return -1;
			}
		}
	else
		{
		if (Biomes->payload.p_byte_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (1)");
			return -1;
			}
		}
	if (!ishere1 || (Sections = nbt_child_find(Level,NBT_LIST,"Sections")) == NULL)
		{
		if ((Sections = nbt_child_new(Level,NBT_LIST,"Sections")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		Sections->payload.p_list = NBT_COMPOUND;
		}
	else
		{
		if (Sections->payload.p_list != NBT_COMPOUND)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (2)");
			return -1;
			}
		}
	//WHEW... now that THAT's over with...
	
	//save to the NBT structure whatever is present in the chunk...
	xPos->payload.p_int = c->x;
	zPos->payload.p_int = c->z;
	if (c->geom != NULL)
		{
		//write biomes...
		memcpy(Biomes->payload.p_byte_array.data,c->geom->biomes,256);
		//find all existing sections...
		for (i=0;i<16;i++)
			s[i] = NULL;
		for (loop = Sections->firstchild; loop != NULL; loop = loop->next_sib)
			{
			if ((probe = nbt_child_find(loop,NBT_BYTE,"Y")) == NULL || probe->payload.p_byte < 0 || probe->payload.p_byte > 15)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (3)");
				return -1;
				}
			s[probe->payload.p_byte] = loop;
			}
		//let's look at each one...
		for (i=0;i<16;i++)
			{
			ishere2 = 0;
			add = 0;
			for (y = i*16; y < (i+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						if (c->geom->blocks[y][z][x] != MCMAP_AIR) //do we need this section at all?
							ishere2 = 1;
						if (c->geom->blocks[y][z][x] > (uint16_t)0xFF) //do we need the 'Add' byte array too?
							add = 1;
						}
					}
				}
			if (!ishere2) //if the section should be empty, just delete it from the NBT structure...
				{
				if (s[i] != NULL)
					nbt_free(nbt_separate(s[i]));
				}
			else
				{
				//make sure this section exists...
				if (s[i] == NULL)
					{
					ishere2 = 0;
					if ((s[i] = nbt_child_new(Sections,NBT_COMPOUND,NULL)) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
						return -1;
						}
					//record Y index in the section...
					if ((probe = nbt_child_new(s[i],NBT_BYTE,"Y")) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
						return -1;
						}
					probe->payload.p_byte = (int8_t)i;
					}
				//allocate or find block data...
				if (!ishere2 || (probe = nbt_child_find(s[i],NBT_BYTE_ARRAY,"Blocks")) == NULL)
					{
					if ((probe = nbt_child_new(s[i],NBT_BYTE_ARRAY,"Blocks")) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
						return -1;
						}
					probe->payload.p_byte_array.size = 4096;
					if ((probe->payload.p_byte_array.data = (int8_t *)calloc(probe->payload.p_byte_array.size,1)) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
						return -1;
						}
					}
				else
					{
					if (probe->payload.p_byte_array.size != 4096)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (4)");
						return -1;
						}
					}
				//write to block data...
				j=0;
				for (y = i*16; y < (i+1)*16; y++)
					{
					for (z=0;z<16;z++)
						{
						for (x=0;x<16;x++)
							{
							probe->payload.p_byte_array.data[j] = (int8_t)((c->geom->blocks[y][z][x])&0x00FF);
							j++;
							}
						}
					}
				if (add)
					{
					//allocate or find 'Add' data...
					if (!ishere2 || (probe = nbt_child_find(s[i],NBT_BYTE_ARRAY,"Add")) == NULL)
						{
						if ((probe = nbt_child_new(s[i],NBT_BYTE_ARRAY,"Add")) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
							return -1;
							}
						probe->payload.p_byte_array.size = 2048;
						if ((probe->payload.p_byte_array.data = (int8_t *)calloc(probe->payload.p_byte_array.size,1)) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
							return -1;
							}
						}
					else
						{
						if (probe->payload.p_byte_array.size != 2048)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (5)");
							return -1;
							}
						}
					//write to 'Add' data...
					j=0;
					for (y = i*16; y < (i+1)*16; y++)
						{
						for (z=0;z<16;z++)
							{
							for (x=0;x<16;x++)
								{
								if (j%2 == 0)
									probe->payload.p_byte_array.data[j/2] = (int8_t)(((c->geom->blocks[y][z][x])>>4)&0xFF00);
								else
									probe->payload.p_byte_array.data[j/2] = probe->payload.p_byte_array.data[j/2] | (int8_t)(((c->geom->blocks[y][z][x])>>8)&0x00FF);
								j++;
								}
							}
						}
					}
				//allocate or find block metadata...
				if (!ishere2 || (probe = nbt_child_find(s[i],NBT_BYTE_ARRAY,"Data")) == NULL)
					{
					if ((probe = nbt_child_new(s[i],NBT_BYTE_ARRAY,"Data")) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
						return -1;
						}
					probe->payload.p_byte_array.size = 2048;
					if ((probe->payload.p_byte_array.data = (int8_t *)calloc(probe->payload.p_byte_array.size,1)) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
						return -1;
						}
					}
				else
					{
					if (probe->payload.p_byte_array.size != 2048)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (6)");
						return -1;
						}
					}
				//write to block metadata...
				j=0;
				for (y = i*16; y < (i+1)*16; y++)
					{
					for (z=0;z<16;z++)
						{
						for (x=0;x<16;x++)
							{
							if (j%2 == 0)
								probe->payload.p_byte_array.data[j/2] = (int8_t)(((c->geom->data[y][z][x])>>4)&0xFF00);
							else
								probe->payload.p_byte_array.data[j/2] = probe->payload.p_byte_array.data[j/2] | (int8_t)(((c->geom->data[y][z][x])>>8)&0x00FF);
							j++;
							}
						}
					}
				}
			}
		}
	if (c->light != NULL)
		{
		//handle height map
		mcmap_chunk_height_update(c);
		if (!ishere1 || (HeightMap = nbt_child_find(Level,NBT_INT_ARRAY,"HeightMap")) == NULL)
			{
			if ((HeightMap = nbt_child_new(Level,NBT_INT_ARRAY,"HeightMap")) == NULL)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
				return -1;
				}
			HeightMap->payload.p_int_array.size = 256;
			if ((HeightMap->payload.p_int_array.data = (int32_t *)calloc(HeightMap->payload.p_int_array.size,4)) == NULL)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
				return -1;
				}
			}
		else
			{
			if (HeightMap->payload.p_int_array.size != 256)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (7)");
				return -1;
				}
			}
		memcpy(HeightMap->payload.p_int_array.data,c->light->height,256*4);
		//handle sky & block light
		for (loop = Sections->firstchild; loop != NULL; loop = loop->next_sib)
			{
			//FIXME - i wonder if we could write 'mcmap_chunk_light_update()' and call it here rather than just deleting everything?
			if (ishere1 && (probe = nbt_child_find(loop,NBT_BYTE_ARRAY,"SkyLight")) != NULL)
				nbt_free(nbt_separate(probe));
			if (ishere1 && (probe = nbt_child_find(loop,NBT_BYTE_ARRAY,"BlockLight")) != NULL)
				nbt_free(nbt_separate(probe));
			}
		}
	if (c->meta != NULL)
		{
		LastUpdate->payload.p_long       = c->meta->mtime;
		TerrainPopulated->payload.p_byte = c->meta->populated;
		InhabitedTime->payload.p_long    = c->meta->itime;
		}
	if (c->special != NULL)
		{
		if (c->special->entities != NULL) //only do stuff if the pointer exists in the chunk struct...
			{
			if (!ishere1 || (Entities = nbt_child_find(Level,NBT_LIST,"Entities")) == NULL) //if the list is absent from the NBT struct...
				{
				//then we have 1 of 2 possible situations: the application pointed it to some random list in another chunk, 
				//or the application created it from scratch, in which case it's probably a standalone root tag...
				if (c->special->entities->parent == NULL) //standalone root tag, safe to transplant...
					{
					//find a place to put it, set structure links...
					if (Level->firstchild != NULL)
						{
						for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
						c->special->entities->prev_sib = loop;
						loop->next_sib = c->special->entities;
						}
					else
						Level->firstchild = c->special->entities;
					c->special->entities->parent = Level;
					}
				else
					{
					//FIXME - maybe copy?
					}
				}
			else //if the list preexists in the NBT struct...
				{
				if (Entities->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (8)");
					return -1;
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list...
				if (Entities != c->special->entities)
					{
					//assuming of course that, once again, the application didn't just point it to some random other chunk's list, and assuming this list isn't empty...
					if (c->special->entities->firstchild != NULL)
						{
						if (c->special->entities->parent == NULL)
							{
							//find a place to put the first child...
							if (Entities->firstchild != NULL)
								{
								for (loop = Entities->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
								c->special->entities->firstchild->prev_sib = loop;
								loop->next_sib = c->special->entities->firstchild;
								}
							else
								Entities->firstchild = c->special->entities->firstchild;
							//reassign the parent for all children...
							for (loop = c->special->entities->firstchild; loop != NULL; loop = loop->next_sib)
								loop->parent = Entities;
							c->special->entities->firstchild = NULL;
							//free the now-empty old parent and reassign our list pointer...
							nbt_free(nbt_separate(c->special->entities));
							c->special->entities = Entities;
							}
						else
							{
							//FIXME - maybe copy?
							}
						}
					}
				}
			}
		if (c->special->tile_entities != NULL)
			{
			if (!ishere1 || (TileEntities = nbt_child_find(Level,NBT_LIST,"TileEntities")) == NULL) //if the list is absent from the NBT struct...
				{
				//then we have 1 of 2 possible situations: the application pointed it to some random list in another chunk, 
				//or the application created it from scratch, in which case it's probably a standalone root tag...
				if (c->special->tile_entities->parent == NULL) //standalone root tag, safe to transplant...
					{
					//find a place to put it, set structure links...
					if (Level->firstchild != NULL)
						{
						for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
						c->special->tile_entities->prev_sib = loop;
						loop->next_sib = c->special->tile_entities;
						}
					else
						Level->firstchild = c->special->tile_entities;
					c->special->tile_entities->parent = Level;
					}
				else
					{
					//FIXME - maybe copy?
					}
				}
			else //if the list preexists in the NBT struct...
				{
				if (TileEntities->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					if (TileEntities->firstchild == NULL)
						TileEntities->payload.p_list = NBT_COMPOUND; //apparently minecraft sometimes saves an empty 'TileEntities' list as a list of NBT_BYTEs ... ? go figure
					else
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (9)");
						return -1;
						}
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list...
				if (TileEntities != c->special->tile_entities)
					{
					//assuming of course that, once again, the application didn't just point it to some random other chunk's list, and assuming this list isn't empty...
					if (c->special->tile_entities->firstchild != NULL)
						{
						if (c->special->tile_entities->parent == NULL)
							{
							//find a place to put the first child...
							if (TileEntities->firstchild != NULL)
								{
								for (loop = TileEntities->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
								c->special->tile_entities->firstchild->prev_sib = loop;
								loop->next_sib = c->special->tile_entities->firstchild;
								}
							else
								TileEntities->firstchild = c->special->tile_entities->firstchild;
							//reassign the parent for all children...
							for (loop = c->special->tile_entities->firstchild; loop != NULL; loop = loop->next_sib)
								loop->parent = TileEntities;
							c->special->tile_entities->firstchild = NULL;
							//free the now-empty old parent and reassign our list pointer...
							nbt_free(nbt_separate(c->special->tile_entities));
							c->special->tile_entities = TileEntities;
							}
						else
							{
							//FIXME - maybe copy?
							}
						}
					}
				}
			}
		if (c->special->tile_ticks != NULL)
			{
			if (!ishere1 || (TileTicks = nbt_child_find(Level,NBT_LIST,"TileTicks")) == NULL) //if the list is absent from the NBT struct...
				{
				//then we have 1 of 2 possible situations: the application pointed it to some random list in another chunk, 
				//or the application created it from scratch, in which case it's probably a standalone root tag...
				if (c->special->tile_ticks->parent == NULL) //standalone root tag, safe to transplant...
					{
					//find a place to put it, set structure links...
					if (Level->firstchild != NULL)
						{
						for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
						c->special->tile_ticks->prev_sib = loop;
						loop->next_sib = c->special->tile_ticks;
						}
					else
						Level->firstchild = c->special->tile_ticks;
					c->special->tile_ticks->parent = Level;
					}
				else
					{
					//FIXME - maybe copy?
					}
				}
			else //if the list preexists in the NBT struct...
				{
				if (TileTicks->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk (10)");
					return -1;
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list...
				if (TileTicks != c->special->tile_ticks)
					{
					//assuming of course that, once again, the application didn't just point it to some random other chunk's list, and assuming this list isn't empty...
					if (c->special->tile_ticks->firstchild != NULL)
						{
						if (c->special->tile_ticks->parent == NULL)
							{
							//find a place to put the first child...
							if (TileTicks->firstchild != NULL)
								{
								for (loop = TileTicks->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
								c->special->tile_ticks->firstchild->prev_sib = loop;
								loop->next_sib = c->special->tile_ticks->firstchild;
								}
							else
								TileTicks->firstchild = c->special->tile_ticks->firstchild;
							//reassign the parent for all children...
							for (loop = c->special->tile_ticks->firstchild; loop != NULL; loop = loop->next_sib)
								loop->parent = TileTicks;
							c->special->tile_ticks->firstchild = NULL;
							//free the now-empty old parent and reassign our list pointer...
							nbt_free(nbt_separate(c->special->tile_ticks));
							c->special->tile_ticks = TileTicks;
							}
						else
							{
							//FIXME - maybe copy?
							}
						}
					}
				}
			}
		}
	return 0;
	}

//save all existing components of the given chunk to the given coords in the given region;
//'rem' is a boolean flag for whether to remember the raw NBT structure on return; return 0 on success and -1 on failure
int mcmap_chunk_write(struct mcmap_region *r, int x, int z, struct mcmap_chunk *c, int rem)
	{
	c->x = x;
	c->z = z;
	
	//save native chunk data to the raw NBT structure...
	if (_mcmap_chunk_nbt_save(c) != 0)
		return -1;
	
	//FIXME - call 'nbt_encode()' and place the results into 'r'
	
	if (!rem)
		{
		nbt_free(c->raw);
		c->raw = NULL;
		}
	
	return 0;
	}

//free all memory allocated in 'mcmap_chunk_read()' or 'mcmap_chunk_new()'
void mcmap_chunk_free(struct mcmap_chunk *c)
	{
	if (c != NULL)
		{
		if (c->raw != NULL)
			nbt_free(c->raw);
		if (c->geom != NULL)
			free(c->geom);
		if (c->light != NULL)
			free(c->light);
		if (c->meta != NULL)
			free(c->meta);
		if (c->special != NULL)
			free(c->special);
		free(c);
		}
	return;
	}
