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
struct mcmap_region *mcmap_region_read(int ix, int iz, char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	struct stat r_stat;
	uint8_t *buff;
	struct mcmap_region *r;
	uint32_t l;
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
				r->dates[z][x] = cswap_32(&(r->header->dates[z][x]));
				//extract big-endian 24-bit integer from r->header->location[z][x].offset
				l = cswap_24(&(r->header->locations[z][x].offset));
				
				//chunk listing should not point anywhere in the file header
				if (l < 2)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed with invalid location %u",r_name,x,z,l);
					return NULL;
					}
				//chunk listing should not point past the end of the file
				i = l*4096;
				if (i+r->header->locations[z][x].sector_count*4096 > r_stat.st_size)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be corrupted: chunk (%d,%d) was listed to inhabit %u 4KiB sectors ending at byte %u; file is only %u bytes long",r_name,x,z,r->header->locations[z][x].sector_count,i+((unsigned int)r->header->locations[z][x].sector_count)*4096,(unsigned int)r_stat.st_size);
					return NULL;
					}
				
				//connect 5-byte chunk header
				r->chunks[z][x].header = (struct mcmap_region_chunk_header *)&(buff[i]);
				//extract big-endian 32-bit integer from r->chunks[z][x].header->length (same location as buff[i])
				r->chunks[z][x].size = cswap_32(&(buff[i]));
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
//on simple geometry inquiries; returns NULL on error
struct mcmap_chunk *mcmap_chunk_read(struct mcmap_region_chunk *rc, mcmap_readmode mode)
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
	if ((t = nbt_find_child(c->raw,NBT_COMPOUND,"Level")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	//read biomes
	if ((p = nbt_find_child(t,NBT_BYTE_ARRAY,"Biomes")) != NULL)
		{
		if (p->payload.p_byte_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		memcpy(c->geom->biomes,p->payload.p_byte_array.data,256);
		}
	//read modified time
	if ((p = nbt_find_child(t,NBT_LONG,"LastUpdate")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	c->meta->mtime = p->payload.p_long;
	//read populated flag
	if ((p = nbt_find_child(t,NBT_BYTE,"TerrainPopulated")) != NULL)
		c->meta->populated = p->payload.p_byte;
	else
		c->meta->populated = 0x00;
	//read inhabited time
	if ((p = nbt_find_child(t,NBT_LONG,"InhabitedTime")) != NULL) //this is supposedly a required tag for a chunk, but older maps (pre-1.6.1) won't have them so we won't sound the alarm
		c->meta->itime = p->payload.p_long;
	
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
		if ((p = nbt_find_child(t,NBT_INT_ARRAY,"HeightMap")) == NULL || p->payload.p_int_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		for (z=0;z<16;z++)
			{
			for (x=0;x<16;x++)
				c->light->height[z][x] = p->payload.p_int_array.data[x+(z*16)];
			}
		//special objects
		if ((c->special->entities = nbt_find_child(t,NBT_LIST,"Entities")) == NULL || (c->special->tile_entities = nbt_find_child(t,NBT_LIST,"TileEntities")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		c->special->tile_ticks = nbt_find_child(t,NBT_LIST,"TileTicks");
		}
	
	//read sections
	if ((t = nbt_find_child(t,NBT_LIST,"Sections")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
		return NULL;
		}
	for (l = t->firstchild; l != NULL; l = l->next_sib)
		{
		
		//determine Y index
		if ((p = nbt_find_child(l,NBT_BYTE,"Y")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		//store Y index for a minute
		y_index = p->payload.p_byte;
		//block IDs
		if ((p = nbt_find_child(l,NBT_BYTE_ARRAY,"Blocks")) == NULL || p->payload.p_byte_array.size != 4096)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		//no memcpy() this time, our arrays are different types/sizes
		for (y = y_index*16; y < (y_index+1)*16; y++)
			{
			for (z=0;z<16;z++)
				{
				for (x=0;x<16;x++)
					c->geom->blocks[y][z][x] = ((uint16_t)p->payload.p_byte_array.data[x+(z*16)+(y*256)])&0x00FF;
				}
			}
		//optional addition to higher 4 bits for every block ID
		if ((p = nbt_find_child(l,NBT_BYTE_ARRAY,"Add")) != NULL)
			{
			if (p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						i = x+(z*16)+(y*256);
						if (i%2 == 0)
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0xF0))<<4)&0x0F00);
						else
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0x0F))<<8)&0x0F00);
						}
					}
				}
			}
		//block metadata
		if ((p = nbt_find_child(l,NBT_BYTE_ARRAY,"Data")) == NULL || p->payload.p_byte_array.size != 2048)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
			return NULL;
			}
		for (y = y_index*16; y < (y_index+1)*16; y++)
			{
			for (z=0;z<16;z++)
				{
				for (x=0;x<16;x++)
					{
					i = x+(z*16)+(y*256);
					if (i%2 == 0)
						c->geom->data[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
					else
						c->geom->data[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
					}
				}
			}
		//optional lighting info
		if (mode == MCMAP_READ_FULL)
			{
			//block-emitted light
			if ((p = nbt_find_child(l,NBT_BYTE_ARRAY,"BlockLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						i = x+(z*16)+(y*256);
						if (i%2 == 0)
							c->light->block[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
						else
							c->light->block[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						}
					}
				}
			//sky-emitted light
			if ((p = nbt_find_child(l,NBT_BYTE_ARRAY,"SkyLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk");
				return NULL;
				}
			for (y = y_index*16; y < (y_index+1)*16; y++)
				{
				for (z=0;z<16;z++)
					{
					for (x=0;x<16;x++)
						{
						i = x+(z*16)+(y*256);
						if (i%2 == 0)
							c->light->sky[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
						else
							c->light->sky[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						}
					}
				}
			}
		
		}
	
	return c;
	}

//free all memory allocated in 'mcmap_chunk_read()' or 'mcmap_chunk_new()'
void mcmap_chunk_free(struct mcmap_chunk *c)
	{
	if (c != NULL)
		{
		if (c->raw != NULL)
			nbt_free_all(c->raw);
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
