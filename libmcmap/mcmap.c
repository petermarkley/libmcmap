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
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include "mcmap.h"
#include "libnbt/nbt.h"
#include "cswap.h"

//assign 'mcmap_region_chunk' pointers, of the chunk at the given coordinates, to the proper places in the memory buffer - assuming correct values everywhere else
void _mcmap_region_chunk_refresh(struct mcmap_region *r, int x, int z)
	{
	uint8_t *b;
	int i;
	//let's avoid dereferencing any NULL pointers, shall we?
	if (r == NULL || r->header == NULL)
		return;
	b = (uint8_t *)r->header;
	i = r->locations[z][x]*4096;
	//connect pointers
	if (r->locations[z][x] >= 2)
		{
		//connect 5-byte chunk header
		r->chunks[z][x].header = (struct mcmap_region_chunk_header *)&(b[i]);
		//'r->chunks[z][x].data' will now point to a block of 'r->chunks[z][x].size' bytes
		r->chunks[z][x].data = &(b[i+5]);
		}
	return;
	}

//check an 'mcmap_region_chunk' for sanity, return 0 if good and -1 if bad
int _mcmap_region_chunk_check(struct mcmap_region *r, int x, int z)
	{
	int i;
	//let's avoid dereferencing any NULL pointers, shall we?
	if (r == NULL || r->header == NULL)
		return 0;
	//chunk listing should not point anywhere in the file header
	if (r->locations[z][x] < 2)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed region: chunk (%d,%d) was listed with invalid location %u",x,z,r->locations[z][x]);
		return -1;
		}
	//chunk listing should not point past the end of the file
	i = r->locations[z][x]*4096;
	if (i+r->header->locations[z][x].sector_count*4096 > r->size)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed region: chunk (%d,%d) was listed to inhabit %u 4KiB sectors ending at byte %u; file is only %u bytes long",x,z,r->header->locations[z][x].sector_count,i+((unsigned int)r->header->locations[z][x].sector_count)*4096,(unsigned int)r->size);
		return -1;
		}
	//listed chunk size should not be larger than the rest of the file
	if (i+5+r->chunks[z][x].size > r->size)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed region: chunk (%d,%d) was listed to be %u bytes when only %u bytes remain of the file",x,z,(unsigned int)r->chunks[z][x].size,((unsigned int)r->size)-(i+5));
		return -1;
		}
	//in fact neither should it be larger than the sector count in the file header
	if (r->chunks[z][x].size+5 > r->header->locations[z][x].sector_count*4096)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed region: chunk (%d,%d) was listed to be %u bytes, which exceeds the %u bytes designated in the header",x,z,(unsigned int)r->chunks[z][x].size,r->header->locations[z][x].sector_count*4096);
		return -1;
		}
	return 0;
	}

//searches the given path to a minecraft region folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; if error returns NULL
struct mcmap_region *mcmap_region_read(int ix, int iz, const char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	struct stat r_stat;
	uint8_t *buff;
	struct mcmap_region *r;
	unsigned int x,z,i;
	
	//resolve filename from regions directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sr.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/r.%d.%d.mca", path, ix, iz);
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
	r->size = r_stat.st_size;
	for (z=0;z<32;z++)
		{
		for (x=0;x<32;x++)
			{
			if (r->header->locations[z][x].sector_count > 0)
				{
				//extract big-endian 32-bit integer from r->header->dates[z][x]
				r->dates[z][x] = cswapr_32(&(r->header->dates[z][x][0]));
				//extract big-endian 24-bit integer from r->header->location[z][x].offset
				r->locations[z][x] = cswapr_24(&(r->header->locations[z][x].offset[0]));
				//connect pointers
				_mcmap_region_chunk_refresh(r,x,z);
				//extract big-endian 32-bit integer for precise chunk size
				r->chunks[z][x].size = cswapr_32(&(r->chunks[z][x].header->length[0]))-1;
				//sanity check
				if (_mcmap_region_chunk_check(r,x,z) == -1)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"\'%s\': %s",r_name,mcmap_error);
					return NULL;
					}
				}
			else
				{
				r->chunks[z][x].header = NULL;
				r->chunks[z][x].size = 0;
				r->chunks[z][x].data = NULL;
				r->locations[z][x] = 0;
				}
			}
		}
	
	return r;
	}

//saves the given region memory structure under the given coordinates in the given minecraft map folder
//returns 0 on success and -1 on failure
int mcmap_region_write(struct mcmap_region *r, int ix, int iz, const char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	uint8_t *buff;
	unsigned int x,z,i;
	
	//let's avoid dereferencing any NULL pointers, shall we?
	if (r == NULL || r->header == NULL)
		return -1;
	
	//write values from parsed native copies to the memory buffer
	for (z=0;z<32;z++)
		{
		for (x=0;x<32;x++)
			{
			if (r->chunks[z][x].header != NULL)
				{
				//sanity check
				if (_mcmap_region_chunk_check(r,x,z) == -1)
					return -1;
				//write big-endian 32-bit integer for precise chunk size
				cswapw_32(&(r->chunks[z][x].header->length[0]),r->chunks[z][x].size+1);
				//write big-endian 24-bit integer to r->header->locations[z][x].offset
				cswapw_24(&(r->header->locations[z][x].offset[0]),r->locations[z][x]);
				//write big-endian 32-bit integer to r->header->dates[z][x]
				cswapw_32(&(r->header->dates[z][x][0]),r->dates[z][x]);
				}
			}
		}
	
	//resolve filename from map directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sregion/r.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/region/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((r_file = fopen(r_name,"w")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on \'%s\': %s",r_name,strerror(errno));
		return -1;
		}
	//write file...
	buff = (uint8_t *)r->header;
	if ((i = fwrite(buff,1,r->size,r_file)) != r->size)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fwrite() returned short item count on \'%s\'",r_name);
		return -1;
		}
	//don't need this anymore...
	fclose(r_file);
	
	return 0;
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
//on simple geometry inquiries; 'rem' is a boolean flag for whether to remember the raw NBT structure; returns NULL on failure
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
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Compound \'Level\' not found as child of root tag");
		return NULL;
		}
	//read biomes
	if ((p = nbt_child_find(t,NBT_BYTE_ARRAY,"Biomes")) != NULL)
		{
		if (p->payload.p_byte_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'Biomes\' did not have size 256");
			return NULL;
			}
		memcpy(c->geom->biomes,p->payload.p_byte_array.data,256);
		}
	//read modified time
	if ((p = nbt_child_find(t,NBT_LONG,"LastUpdate")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Long \'LastUpdate\' not found");
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
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int \'xPos\' not found");
		return NULL;
		}
	c->x = p->payload.p_int;
	if ((p = nbt_child_find(t,NBT_INT,"zPos")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int \'zPos\' not found");
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
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int Array \'HeightMap\' with size 256 not found");
			return NULL;
			}
		memcpy(c->light->height,p->payload.p_int_array.data,256*4);
		//special objects
		if ((c->special->entities = nbt_child_find(t,NBT_LIST,"Entities")) == NULL || (c->special->tile_entities = nbt_child_find(t,NBT_LIST,"TileEntities")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: one or both of Lists \'Entities\' & \'TileEntities\' not found");
			return NULL;
			}
		c->special->tile_ticks = nbt_child_find(t,NBT_LIST,"TileTicks");
		}
	
	//read sections
	if ((t = nbt_child_find(t,NBT_LIST,"Sections")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: List \'Sections\' not found");
		return NULL;
		}
	for (l = t->firstchild; l != NULL; l = l->next_sib)
		{
		
		//determine Y index
		if ((p = nbt_child_find(l,NBT_BYTE,"Y")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte \'Y\' not found");
			return NULL;
			}
		//store Y index for a minute
		y_index = p->payload.p_byte;
		//block IDs
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Blocks")) == NULL || p->payload.p_byte_array.size != 4096)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'Blocks\' with size 4096 not found");
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'Add\' did not have size 2048");
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
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0x0F))<<8)&0x0F00);
						else
							c->geom->blocks[y][z][x] = c->geom->blocks[y][z][x] | ((((uint16_t)(p->payload.p_byte_array.data[i/2]&0xF0))<<4)&0x0F00);
						i++;
						}
					}
				}
			}
		//block metadata
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Data")) == NULL || p->payload.p_byte_array.size != 2048)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'Data\' with size 2048 not found");
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
						c->geom->data[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
					else
						c->geom->data[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'Block Light\' with size 2048 not found");
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
							c->light->block[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						else
							c->light->block[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
						i++;
						}
					}
				}
			//sky-emitted light
			if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"SkyLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array \'SkyLight\' with size 2048 not found");
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
							c->light->sky[y][z][x] =  (p->payload.p_byte_array.data[i/2]&0x0F);
						else
							c->light->sky[y][z][x] = ((p->payload.p_byte_array.data[i/2]&0xF0)>>4)&0x0F;
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

//return 1 if light passes through the given block totally unaltered, otherwise 0
int _mcmap_chunk_height_update_test(mcmap_blockid i)
	{
	switch (i)
		{
		case MCMAP_AIR:
		case MCMAP_GLASS:
		case MCMAP_GLASS_PANE:
			return 1;
			break;
		default: return 0; break;
		}
	return 0;
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
				for (y=256; y>0 && _mcmap_chunk_height_update_test(c->geom->blocks[y-1][z][x]); y--);
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
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array \'Biomes\' was not size 256");
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
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; List \'Sections\' was not type Compound");
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Compound child of List \'Sections\' did not have a \'Y\' index in range");
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array \'Blocks\' was not size 4096");
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
							snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array \'Add\' was not size 2048");
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
									probe->payload.p_byte_array.data[j/2] = (int8_t)(((c->geom->blocks[y][z][x])>>8)&0x000F);
								else
									probe->payload.p_byte_array.data[j/2] = probe->payload.p_byte_array.data[j/2] | (int8_t)(((c->geom->blocks[y][z][x])>>4)&0x00F0);
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array \'Data\' was not size 2048");
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
								probe->payload.p_byte_array.data[j/2] = (int8_t)(((c->geom->data[y][z][x])<<0)&0x0F);
							else
								probe->payload.p_byte_array.data[j/2] = probe->payload.p_byte_array.data[j/2] | (int8_t)(((c->geom->data[y][z][x])<<4)&0xF0);
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Int Array \'HeightMap\' was not size 256");
				return -1;
				}
			}
		memcpy(HeightMap->payload.p_int_array.data,c->light->height,256*4);
		//handle sky & block light
		/*for (loop = Sections->firstchild; loop != NULL; loop = loop->next_sib)
			{
			//FIXME - i wonder if we could write 'mcmap_chunk_light_update()' and call it here rather than just deleting everything?
			if (ishere1 && (probe = nbt_child_find(loop,NBT_BYTE_ARRAY,"SkyLight")) != NULL)
				nbt_free(nbt_separate(probe));
			if (ishere1 && (probe = nbt_child_find(loop,NBT_BYTE_ARRAY,"BlockLight")) != NULL)
				nbt_free(nbt_separate(probe));
			}*/
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
					snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; List \'Entities\' was not type Compound");
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; non-empty \'TileEntities\' List was not type Compound");
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
					snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; List \'TileTicks\' was not type Compound");
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
	uint8_t *b = NULL;
	uint8_t *m = (uint8_t *)r->header;
	int s, i, d, lx,lz, f, e;
	
	//save native chunk data to the raw NBT structure
	c->x = x;
	c->z = z;
	if (_mcmap_chunk_nbt_save(c) != 0)
		return -1;
	
	//save raw NBT structure to binary memory buffer
	if ((s = nbt_encode(c->raw,&b,MCMAP_COMPRESSION)) == -1)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return -1;
		}
	if (!rem)
		{
		nbt_free(c->raw);
		c->raw = NULL;
		}
	
	//negotiate a place for the memory buffer in the 'mcmap_region' struct, and save the buffer there
	d = (int)ceil(((double)s)/4096.0) - r->header->locations[z][x].sector_count;
	e = (int)ceil(((double)r->size)/4096.0);
	r->header->locations[z][x].sector_count = (uint8_t)((int)ceil(((double)s)/4096.0));
	if (r->chunks[z][x].header != NULL) //chunk already exists
		{
		//make sure there's the right amount of space for it
		if (d != 0)
			{
			if (d > 0) //new chunk is larger than old chunk
				{
				//expand memory first
				r->size += d*4096;
				if ((r->header = (struct mcmap_region_header *)realloc(r->header,r->size)) == NULL)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
					return -1;
					}
				//shuffle things starting at the end and moving backwards
				for (i = e-1; i > r->locations[z][x]; i--)
					{
					//find out which chunk this is and slide it toward the end
					f=0;
					for (lz=15; lz>=0 && !f; lz--)
						{
						for (lx=15; lx>=0 && !f; lx--)
							{
							if (r->locations[lz][lx] == i)
								{
								f=1;
								memmove(&(m[(i+d)*4096]),&(m[i*4096]),r->chunks[lz][lx].size+5);
								r->locations[lz][lx] += d;
								}
							}
						}
					}
				}
			else //new chunk is smaller than old chunk
				{
				//shuffle things starting here and moving forwards...
				for (i = r->locations[z][x]+1; i < e; i++)
					{
					//find out which chunk this is and slide it toward the end
					f=0;
					for (lz=0; lz<16 && !f; lz++)
						{
						for (lx=0; lx<16 && !f; lx++)
							{
							if (r->locations[lz][lx] == i)
								{
								f=1;
								memmove(&(m[(i+d)*4096]),&(m[i*4096]),r->chunks[lz][lx].size+5);
								r->locations[lz][lx] += d;
								}
							}
						}
					}
				//shrink memory when we're done...
				r->size += d*4096;
				if ((r->header = (struct mcmap_region_header *)realloc(r->header,r->size)) == NULL)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
					return -1;
					}
				}
			}
		}
	else //chunk doesn't exist; we must create it
		{
		r->size += r->header->locations[z][x].sector_count*4096;
		if ((r->header = (struct mcmap_region_header *)realloc(r->header,r->size)) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
			return -1;
			}
		r->locations[z][x] = e-1; //put it at the end
		}
	//restore all chunk pointers after potentially invalidating them with 'realloc()' or definitely invalidating them with 'memmove()'
	for (lz=0;lz<16;lz++)
		for (lx=0;lx<16;lx++)
			_mcmap_region_chunk_refresh(r,lx,lz);
	//write chunk
	memcpy(r->chunks[z][x].data,b,s);
	r->chunks[z][x].size = s;
	r->chunks[z][x].header->compression = (uint8_t)MCMAP_COMPRESSION;
	free(b);
	
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

//worker function for 'mcmap_level_read()', called for each of 'mcmap_level's members 'overworld', 'nether', & 'end'
//returns 0 on success and -1 on failure
int _mcmap_level_world_read(const char *path, struct mcmap_level_world *w, mcmap_readmode mode, int rem)
	{
	DIR *d;
	struct dirent *e;
	int minx,minz,maxx,maxz;
	int x,z, ix,iz, lx,lz;
	int first;
	w->size_x = 0;
	w->size_z = 0;
	
	//open directory stream
	if ((d = opendir(path)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"opendir(\"%s\") returned NULL",path);
		return -1;
		}
	//scan directory to determine the world limits
	first = 1;
	while ((e = readdir(d)) != NULL)
		{
		if (sscanf(e->d_name,"r.%d.%d.mca",&x,&z) == 2)
			{
			if (first)
				{
				first = 0;
				minx = maxx = x;
				minz = maxz = z;
				}
			else
				{
				if (x < minx)
					minx = x;
				if (x > maxx)
					maxx = x;
				if (z < minz)
					minz = z;
				if (z > maxz)
					maxz = z;
				}
			}
		}
	
	//initialize world
	w->start_x = minx;
	w->start_z = minz;
	w->size_x = maxx-minx+1;
	w->size_z = maxz-minz+1;
	if ((w->regions = (struct mcmap_level_region ***)calloc(w->size_z,sizeof(struct mcmap_level_region **))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return -1;
		}
	for (z=0; z < w->size_z; z++)
		{
		if ((w->regions[z] = (struct mcmap_level_region **)calloc(w->size_x,sizeof(struct mcmap_level_region *))) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return -1;
			}
		for (x=0; x < w->size_x; x++)
			{
			if ((w->regions[z][x] = (struct mcmap_level_region *)calloc(1,sizeof(struct mcmap_level_region))) == NULL)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
				return -1;
				}
			}
		}
	
	//populate world
	if (mode == MCMAP_READ_FULL)
		{
		rewinddir(d);
		while ((e = readdir(d)) != NULL)
			{
			if (sscanf(e->d_name,"r.%d.%d.mca",&x,&z) == 2)
				{
				ix = x - w->start_x;
				iz = z - w->start_z;
				//read region
				if ((w->regions[iz][ix]->raw = mcmap_region_read(x,z,path)) != NULL)
					{
					//read chunks
					for (lz=0;lz<16;lz++)
						{
						for (lx=0;lx<16;lx++)
							w->regions[iz][ix]->chunks[lz][lx] = mcmap_chunk_read(&(w->regions[iz][ix]->raw->chunks[lz][lx]),mode,rem);
						}
					}
				}
			}
		}
	
	//clean up
	if (closedir(d) == -1)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"closedir() failed");
		return -1;
		}
	if (!rem)
		{
		for (z=0; z < w->size_z; z++)
			{
			for (x=0; x < w->size_x; x++)
				{
				if (w->regions[z][x] != NULL && w->regions[z][x]->raw != NULL)
					mcmap_region_free(w->regions[z][x]->raw);
				}
			}
		}
	return 0;
	}

//creates and returns a level struct by reading the minecraft map at the given path;
// 'mode' should be MCMAP_READ_PARTIAL to let the caller cherry-pick regions and chunks with
// 'mcmap_region_read()' and 'mcmap_chunk_read()', or MCMAP_READ_FULL to read everything
// (warning: may consume LOTS of memory); 'rem' is a boolean flag for whether to remember
// the raw data at each stage; returns NULL on failure
struct mcmap_level *mcmap_level_read(const char *path, mcmap_readmode mode, int rem)
	{
	struct mcmap_level *l;
	char on[MCMAP_MAXSTR];
	char nn[MCMAP_MAXSTR];
	char en[MCMAP_MAXSTR];
	char ln[MCMAP_MAXSTR];
	int i;
	
	//resolve items from map directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		{
		snprintf(on,MCMAP_MAXSTR,"%sregion/",path);
		snprintf(nn,MCMAP_MAXSTR,"%sDIM-1/",path);
		snprintf(en,MCMAP_MAXSTR,"%sDIM1/",path);
		snprintf(ln,MCMAP_MAXSTR,"%slevel.dat",path);
		}
	else
		{
		snprintf(on,MCMAP_MAXSTR,"%s/regions/",path);
		snprintf(nn,MCMAP_MAXSTR,"%s/DIM-1/",path);
		snprintf(en,MCMAP_MAXSTR,"%s/DIM1/",path);
		snprintf(ln,MCMAP_MAXSTR,"%s/level.dat",path);
		}
	//allocate level...
	if ((l = (struct mcmap_level *)calloc(1,sizeof(struct mcmap_level))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//populate level...
	if (_mcmap_level_world_read(on,&(l->overworld),mode,rem) == -1)
		return NULL;
	if (_mcmap_level_world_read(nn,&(l->nether),mode,rem) == -1)
		return NULL;
	if (_mcmap_level_world_read(en,&(l->end),mode,rem) == -1)
		return NULL;
	//read 'level.dat' file...
	if ((l->meta = nbt_file_read(ln)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	
	return l;
	}

//worker function for 'mcmap_level_free()', called for each of 'mcmap_level's members 'overworld', 'nether', & 'end'
void _mcmap_level_world_free(struct mcmap_level_world *w)
	{
	int x,z, lx,lz;
	if (w == NULL)
		return;
	for (z=0; z < w->size_z; z++)
		{
		for (x=0; x < w->size_x; x++)
			{
			for (lz=0;lz<16;lz++)
				{
				for (lx=0;lx<16;lx++)
					{
					if (w->regions[z][x]->chunks[lz][lx] != NULL)
						mcmap_chunk_free(w->regions[z][x]->chunks[lz][lx]);
					}
				}
			if (w->regions[z][x]->raw != NULL)
				mcmap_region_free(w->regions[z][x]->raw);
			free(w->regions[z][x]);
			}
		free(w->regions[z]);
		}
	free(w->regions);
	return;
	}

//free all memory allocated in 'mcmap_level_read()' or 'mcmap_level_new()'
void mcmap_level_free(struct mcmap_level *l)
	{
	if (l == NULL)
		return;
	_mcmap_level_world_free(&(l->overworld));
	_mcmap_level_world_free(&(l->nether));
	_mcmap_level_world_free(&(l->end));
	if (l->meta != NULL)
		nbt_free(l->meta);
	return;
	}
