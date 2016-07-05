//---------------------------------------------------------------------
//                         libmcmap
//               Minecraft map reading & writing
//      < http://minecraft.gamepedia.com/Level_Format >
// 
// Copyright 2013-2016 by Peter Markley <quartz@malexmedia.net>.
// Distributed under the terms of the Lesser GNU General Public License.
// 
// Minecraft is a trademark of Mojang AB. Libmcmap is developed by a
// third party under the terms provided at
// < http://minecraft.net/terms >.
// 
// This file is part of libmcmap.
// 
// Libmcmap is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
// 
// Libmcmap is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Lesser GNU General Public License for more details.
// 
// You should have received a copy of the Lesser GNU General Public
// License along with libmcmap.  If not, see
// < http://www.gnu.org/licenses/ >.
// 
//----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#define __MCMAP_EXTERN
#include "mcmap.h"
#include "nbt.h"
#include "cswap.h"
#ifdef __MCMAP_DEBUG
	#include "memdb.h"
#endif

//worker function for 'mcmap_level_read()', called for each of 'mcmap_level's members 'overworld', 'nether', & 'end'
//returns 0 if good and -1 if bad
int _mcmap_level_world_memcheck(struct mcmap_level_world *w, const char *world)
	{
	#ifdef __MCMAP_DEBUG
	struct mcmap_chunk *c;
	int ret1,ret2;
	int x,z, cx,cz;
	if (w == NULL) return 0;
	if (w->path != NULL && (ret1 = memdb_check(w->path)) != (ret2 = strlen(w->path)+1))
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->path'=%p) returned %d (expected string of size %d)",world,w->path,ret1,ret2);
		return -1;
		}
	ret1 = (w->size_x != 0 && w->size_z != 0);
	if ((w->regions == NULL && ret1) || (!ret1 && w->regions != NULL))
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'regions' NULL status disagrees with recorded size");
		return -1;
		}
	if (w->regions != NULL && (ret1 = memdb_check(w->regions)) != (ret2 = w->size_z*sizeof(struct mcmap_level_region **)))
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions'=%p) returned %d (expected %d pointers totaling %d bytes)",world,w->regions,ret1,w->size_z,ret2);
		return -1;
		}
	for (z=0; z < w->size_z; z++)
		{
		if (w->regions[z] == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"'l->%s->regions[%d]' exists but points to NULL",world,z);
			return -1;
			}
		if ((ret1 = memdb_check(w->regions[z])) != (ret2 = w->size_x*sizeof(struct mcmap_level_region *)))
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d]'=%p) returned %d (expected %d pointers totaling %d bytes)",world,z,w->regions[z],ret1,w->size_x,ret2);
			return -1;
			}
		for (x=0; x < w->size_x; x++)
			{
			if (w->regions[z][x] == NULL)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"'l->%s->regions[%d][%d]' exists but points to NULL",world,z,x);
				return -1;
				}
			if ((ret1 = memdb_check(w->regions[z][x])) != (ret2 = sizeof(struct mcmap_level_region)))
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]'=%p) returned %d (expected 'struct mcmap_level_region' size %d)",world,z,x,w->regions[z][x],ret1,ret2);
				return -1;
				}
			if (w->regions[z][x]->raw != NULL)
				{
				if ((ret1 = memdb_check(w->regions[z][x]->raw)) != (ret2 = sizeof(struct mcmap_region)))
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->raw'=%p) returned %d (expected 'struct mcmap_region' size %d)",world,z,x,w->regions[z][x]->raw,ret1,ret2);
					return -1;
					}
				if ((ret1 = memdb_check(w->regions[z][x]->raw->header)) != (ret2 = w->regions[z][x]->raw->size))
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->raw->header'=%p) returned %d (expected %d bytes)",world,z,x,w->regions[z][x]->raw->header,ret1,ret2);
					return -1;
					}
				}
			for (cz=0;cz<32;cz++)
				{
				for (cx=0;cx<32;cx++)
					{
					if ((c = w->regions[z][x]->chunks[cz][cx]) != NULL)
						{
						if ((ret1 = memdb_check(c)) != (ret2 = sizeof(struct mcmap_chunk)))
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->chunks[%d][%d]'=%p) returned %d (expected 'struct mcmap_chunk' size %d)",world,z,x,cz,cx,c,ret1,ret2);
							return -1;
							}
						if (c->raw != NULL && nbt_memcheck(c->raw) == -1)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"%s on 'l->%s->regions[%d][%d]->chunks[%d][%d]->raw': %s",NBT_LIBNAME,world,z,x,cz,cx,nbt_error);
							return -1;
							}
						if (c->geom != NULL && (ret1 = memdb_check(c->geom)) != (ret2 = sizeof(struct mcmap_chunk_geom)))
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->chunks[%d][%d]->geom'=%p) returned %d (expected 'struct mcmap_chunk_goem' size %d)",world,z,x,cz,cx,c->geom,ret1,ret2);
							return -1;
							}
						if (c->light != NULL && (ret1 = memdb_check(c->light)) != (ret2 = sizeof(struct mcmap_chunk_light)))
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->chunks[%d][%d]->light'=%p) returned %d (expected 'struct mcmap_chunk_light' size %d)",world,z,x,cz,cx,c->light,ret1,ret2);
							return -1;
							}
						if (c->meta != NULL && (ret1 = memdb_check(c->meta)) != (ret2 = sizeof(struct mcmap_chunk_meta)))
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->chunks[%d][%d]->meta'=%p) returned %d (expected 'struct mcmap_chunk_meta' size %d)",world,z,x,cz,cx,c->meta,ret1,ret2);
							return -1;
							}
						if (c->special != NULL)
							{
							if ((ret1 = memdb_check(c->special)) != (ret2 = sizeof(struct mcmap_chunk_special)))
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->%s->regions[%d][%d]->chunks[%d][%d]->special'=%p) returned %d (expected 'struct mcmap_chunk_special' size %d)",world,z,x,cz,cx,c->special,ret1,ret2);
								return -1;
								}
							if (c->special->entities != NULL && nbt_memcheck(c->special->entities) == -1)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"%s on 'l->%s->regions[%d][%d]->chunks[%d][%d]->special->entities': %s",NBT_LIBNAME,world,z,x,cz,cx,nbt_error);
								return -1;
								}
							if (c->special->tile_entities != NULL && nbt_memcheck(c->special->tile_entities) == -1)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"%s on 'l->%s->regions[%d][%d]->chunks[%d][%d]->special->tile_entities': %s",NBT_LIBNAME,world,z,x,cz,cx,nbt_error);
								return -1;
								}
							if (c->special->tile_ticks != NULL && nbt_memcheck(c->special->tile_ticks) == -1)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"%s on 'l->%s->regions[%d][%d]->chunks[%d][%d]->special->tile_ticks': %s",NBT_LIBNAME,world,z,x,cz,cx,nbt_error);
								return -1;
								}
							}
						}
					}
				}
			}
		}
	#endif
	return 0;
	}

//compile with '-D __MCMAP_DEBUG' to use; sanity check all allocated memory spaces involved in the given level struct
//return 0 if good and -1 if bad
int mcmap_level_memcheck(struct mcmap_level *l)
	{
	#ifdef __MCMAP_DEBUG
	int ret1,ret2;
	if (l == NULL) return 0;
	if ((ret1 = memdb_check(l)) != (ret2 = sizeof(struct mcmap_level)))
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l'=%p) returned %d (expected 'struct mcmap_level' size %d)",l,ret1,ret2);
		return -1;
		}
	if (l->path != NULL && (ret1 = memdb_check(l->path)) != (ret2 = strlen(l->path)+1))
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"memdb_check('l->path'=%p) returned %d (expected string of size %d)",l->path,ret1,ret2);
		return -1;
		}
	if (l->meta != NULL && nbt_memcheck(l->meta) == -1)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s on 'l->meta': %s",NBT_LIBNAME,nbt_error);
		return -1;
		}
	if (_mcmap_level_world_memcheck(&(l->overworld),"overworld") == -1) return -1;
	if (_mcmap_level_world_memcheck(&(l->nether),"nether") == -1) return -1;
	if (_mcmap_level_world_memcheck(&(l->end),"end") == -1) return -1;
	#endif
	return 0;
	}

//return level of light emitted by the given block type < http://minecraft.gamepedia.com/Light#Light-emitting_blocks >
uint8_t _mcmap_light_update_emit(mcmap_blockid i)
	{
	switch (i)
		{
		case MCMAP_BEACON:
		case MCMAP_END_PORTAL:
		case MCMAP_FIRE:
		case MCMAP_GLOWSTONE:
		case MCMAP_JACK_O_LANTERN:
		case MCMAP_FLOWING_LAVA:
		case MCMAP_LAVA:
		case MCMAP_LIT_REDSTONE_LAMP:
		case MCMAP_SEA_LANTERN:
		case MCMAP_END_GATEWAY:
			return 0x0f; //light level 15
			break;
		case MCMAP_TORCH:
		case MCMAP_END_ROD:
			return 0x0e; //light level 14
			break;
		case MCMAP_LIT_FURNACE:
			return 0x0d; //light level 13
			break;
		case MCMAP_NETHER_PORTAL:
			return 0x0b; //light level 12
			break;
		case MCMAP_GLOWING_REDSTONE_ORE:
		case MCMAP_LIT_REDSTONE_REPEATER:
		case MCMAP_LIT_REDSTONE_COMPARATOR:
			return 0x09; //light level 9
			break;
		case MCMAP_ENDER_CHEST:
		case MCMAP_LIT_REDSTONE_TORCH:
			return 0x07; //light level 7
			break;
		case MCMAP_MAGMA:
			return 0x03; //light level 3
			break;
		case MCMAP_BREWING_STAND:
		case MCMAP_BROWN_MUSHROOM:
		case MCMAP_DRAGON_EGG:
		case MCMAP_END_PORTAL_FRAME:
			return 0x01; //light level 1
			break;
		default: return 0x00; break;
		}
	return 0x00;
	}

//return amount of extinction applied by the given block type < http://minecraft.gamepedia.com/Opacity >
uint8_t _mcmap_light_update_extinct(mcmap_blockid i)
	{
	switch (i)
		{
		case MCMAP_AIR:
		case MCMAP_GLASS:
		case MCMAP_GLASS_PANE:
		case MCMAP_STAINED_GLASS:
		case MCMAP_STAINED_GLASS_PANE:
		case MCMAP_CARPET:
		case MCMAP_LEAVES:
		case MCMAP_LEAVES_2:
		case MCMAP_PISTON:
		case MCMAP_STICKY_PISTON:
		case MCMAP_PISTON_EXT:
		case MCMAP_GLOWSTONE:
		case MCMAP_SPAWNER:
		case MCMAP_CAULDRON:
		case MCMAP_LADDER:
		case MCMAP_FENCE:
		case MCMAP_GATE:
		case MCMAP_NETHER_BRICK_FENCE:
		case MCMAP_COBBLESTONE_WALL:
		case MCMAP_IRON_BARS:
		case MCMAP_PLACED_CAKE:
		case MCMAP_BED:
		case MCMAP_WOOD_DOOR:
		case MCMAP_IRON_DOOR:
		case MCMAP_TRAPDOOR:
		case MCMAP_IRON_TRAPDOOR:
		case MCMAP_UNLIT_REDSTONE_REPEATER:
		case MCMAP_LIT_REDSTONE_REPEATER:
		case MCMAP_UNLIT_REDSTONE_COMPARATOR:
		case MCMAP_LIT_REDSTONE_COMPARATOR:
		case MCMAP_CHEST:
		case MCMAP_ENDER_CHEST:
		case MCMAP_TRAPPED_CHEST:
		case MCMAP_ENCHANTING_TABLE:
		case MCMAP_ANVIL:
		case MCMAP_BREWING_STAND:
		case MCMAP_END_PORTAL_FRAME:
		case MCMAP_HOPPER:
		case MCMAP_DAYLIGHT_SENSOR:
		case MCMAP_INVERTED_DAYLIGHT_SENSOR:
		case MCMAP_SNOW:
		case MCMAP_LILY_PAD:
		case MCMAP_DRAGON_EGG:
		case MCMAP_RAIL:
		case MCMAP_POWERED_RAIL:
		case MCMAP_DETECTOR_RAIL:
		case MCMAP_ACTIVATOR_RAIL:
		case MCMAP_LEVER:
		case MCMAP_STONE_PRESSURE_PLATE:
		case MCMAP_WOOD_PRESSURE_PLATE:
		case MCMAP_LIGHT_PRESSURE_PLATE:
		case MCMAP_HEAVY_PRESSURE_PLATE:
		case MCMAP_STONE_BUTTON:
		case MCMAP_WOOD_BUTTON:
		case MCMAP_TRIPWIRE_HOOK:
		case MCMAP_TRIPWIRE:
		case MCMAP_REDSTONE_WIRE:
		case MCMAP_UNLIT_REDSTONE_TORCH:
		case MCMAP_LIT_REDSTONE_TORCH:
		case MCMAP_COBWEB:
		case MCMAP_TORCH:
		case MCMAP_SIGN:
		case MCMAP_WALL_SIGN:
		case MCMAP_FIRE:
		case MCMAP_NETHER_PORTAL:
		case MCMAP_END_PORTAL:
		case MCMAP_CACTUS:
		case MCMAP_SUGAR_CANE:
		case MCMAP_WHEAT:
		case MCMAP_CARROTS:
		case MCMAP_POTATOES:
		case MCMAP_NETHER_WART:
		case MCMAP_DANDELION:
		case MCMAP_POPPY:
		case MCMAP_BROWN_MUSHROOM:
		case MCMAP_RED_MUSHROOM:
		case MCMAP_SAPLING:
		case MCMAP_TALL_GRASS:
		case MCMAP_DEAD_BUSH:
		case MCMAP_PUMPKIN_STEM:
		case MCMAP_MELON_STEM:
		case MCMAP_VINES:
		case MCMAP_COCOA_POD:
		case MCMAP_FLOWER_POT:
		case MCMAP_HEAD:
		case MCMAP_SLIME_BLOCK:
		case MCMAP_BARRIER:
		case MCMAP_SEA_LANTERN:
		case MCMAP_DOUBLE_PLANT:
		case MCMAP_STANDING_BANNER:
		case MCMAP_WALL_BANNER:
		case MCMAP_SPRUCE_GATE:
		case MCMAP_BIRCH_GATE:
		case MCMAP_JUNGLE_GATE:
		case MCMAP_DARK_OAK_GATE:
		case MCMAP_ACACIA_GATE:
		case MCMAP_SPRUCE_FENCE:
		case MCMAP_BIRCH_FENCE:
		case MCMAP_JUNGLE_FENCE:
		case MCMAP_DARK_OAK_FENCE:
		case MCMAP_ACACIA_FENCE:
		case MCMAP_SPRUCE_WOOD_DOOR:
		case MCMAP_BIRCH_WOOD_DOOR:
		case MCMAP_JUNGLE_WOOD_DOOR:
		case MCMAP_ACACIA_WOOD_DOOR:
		case MCMAP_DARK_OAK_WOOD_DOOR:
		case MCMAP_END_ROD:
		case MCMAP_CHORUS_PLANT:
		case MCMAP_CHORUS_FLOWER:
		case MCMAP_BEETROOTS:
		case MCMAP_STRUCTURE_VOID:
			return 0x01;
			break;
		case MCMAP_FLOWING_WATER:
		case MCMAP_WATER:
		case MCMAP_ICE:
		case MCMAP_FROSTED_ICE:
			return 0x03;
			break;
		default: return 0x0f; break;
		}
	return 0x0f;
	}

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
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed region: chunk (%d,%d) was listed to be %u bytes, which exceeds the %u bytes designated in the header",x,z,(unsigned int)r->chunks[z][x].size+5,r->header->locations[z][x].sector_count*4096);
		return -1;
		}
	return 0;
	}

//allocates a bare minimum region, which can be passed to 'mcmap_region_write()'; returns NULL on failure
struct mcmap_region *mcmap_region_new(void)
	{
	struct mcmap_region *r;
	if ((r = (struct mcmap_region *)calloc(1,sizeof(struct mcmap_region))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	r->size = 8192;
	if ((r->header = (struct mcmap_region_header *)calloc(r->size,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	return r;
	}

//searches the given path to a minecraft region folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; returns NULL on failure
struct mcmap_region *mcmap_region_read(int ix, int iz, const char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	struct stat r_stat;
	uint8_t *buff;
	struct mcmap_region *r;
	unsigned int x,z,i;
	if (path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' is NULL");
		return NULL;
		}
	
	//resolve filename from regions directory...
	i = strlen(path);
	if (i==0 || path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sr.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((r_file = fopen(r_name,"rb")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on '%s': %s",r_name,strerror(errno));
		return NULL;
		}
	//determine filesize...
	if (fstat(fileno(r_file),&r_stat) != 0)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fstat() on '%s': %s",r_name,strerror(errno));
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
		snprintf(mcmap_error,MCMAP_MAXSTR,"fread() encountered %s on the last %d requested bytes of '%s'",(ferror(r_file)?"an error":"EOF"),(unsigned int)r_stat.st_size-i,r_name);
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
					snprintf(mcmap_error,MCMAP_MAXSTR,"'%s': %s",r_name,mcmap_error);
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
	if (path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' is NULL");
		return -1;
		}
	
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
	i = strlen(path);
	if (i==0 || path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sr.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((r_file = fopen(r_name,"wb")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on '%s': %s",r_name,strerror(errno));
		return -1;
		}
	//write file...
	buff = (uint8_t *)r->header;
	if ((i = fwrite(buff,1,r->size,r_file)) != r->size)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fwrite() returned short item count on '%s'",r_name);
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

//update height map based on geometry
void mcmap_chunk_height_update(struct mcmap_chunk *c)
	{
	int x,y,z;
	if (c != NULL && c->geom != NULL && c->light != NULL)
		{
		for (z=0;z<16;z++)
			{
			for (x=0;x<16;x++)
				{
				for (y=256; y>0 && c->geom->blocks[y-1][z][x] != MCMAP_LEAVES && c->geom->blocks[y-1][z][x] != MCMAP_LEAVES_2 && c->geom->blocks[y-1][z][x] != MCMAP_GLOWSTONE && c->geom->blocks[y-1][z][x] != MCMAP_COBWEB && _mcmap_light_update_extinct(c->geom->blocks[y-1][z][x]) == 0x01; y--);
				c->light->height[z][x] = y;
				}
			}
		}
	return;
	}

//allocate a chunk and record the given global chunk coordinates; 'mode' should be MCMAP_PARTIAL
//to only create geometry and chunk metadata, MCMAP_FULL to create everything; returns NULL on failure
struct mcmap_chunk *mcmap_chunk_new(int x, int z, mcmap_mode mode)
	{
	struct mcmap_chunk *c;
	if ((c = (struct mcmap_chunk *)calloc(1,sizeof(struct mcmap_chunk))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	c->x = x; c->z = z;
	if ((c->geom = (struct mcmap_chunk_geom *)calloc(1,sizeof(struct mcmap_chunk_geom))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	if ((c->meta = (struct mcmap_chunk_meta *)calloc(1,sizeof(struct mcmap_chunk_meta))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	if (mode == MCMAP_FULL)
		{
		if ((c->light = (struct mcmap_chunk_light *)calloc(1,sizeof(struct mcmap_chunk_light))) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return NULL;
			}
		if ((c->special = (struct mcmap_chunk_special *)calloc(1,sizeof(struct mcmap_chunk_special))) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return NULL;
			}
		}
	return c;
	}

//takes an individual chunk from a 'struct mcmap_region,' returns a parsed 'mcmap_chunk;'
//'mode' should be MCMAP_FULL for fully populated chunk, MCMAP_PARTIAL to save memory
//on simple geometry inquiries; 'rem' is a boolean flag for whether to remember the raw NBT structure; returns NULL on failure
struct mcmap_chunk *mcmap_chunk_read(struct mcmap_region_chunk *rc, mcmap_mode mode, int rem)
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
	if ((c = mcmap_chunk_new(0,0,mode)) == NULL)
		return NULL;
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
	
	//drill past root container
	if ((t = nbt_child_find(c->raw,NBT_COMPOUND,"Level")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Compound 'Level' not found as child of root tag");
		return NULL;
		}
	//read biomes
	if ((p = nbt_child_find(t,NBT_BYTE_ARRAY,"Biomes")) != NULL)
		{
		if (p->payload.p_byte_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'Biomes' did not have size 256");
			return NULL;
			}
		memcpy(c->geom->biomes,p->payload.p_byte_array.data,256);
		}
	//read modified time
	if ((p = nbt_child_find(t,NBT_LONG,"LastUpdate")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Long 'LastUpdate' not found");
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
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int 'xPos' not found");
		return NULL;
		}
	c->x = p->payload.p_int;
	if ((p = nbt_child_find(t,NBT_INT,"zPos")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int 'zPos' not found");
		return NULL;
		}
	c->z = p->payload.p_int;
	
	//read optional data
	if (mode == MCMAP_FULL)
		{
		//height map
		if ((p = nbt_child_find(t,NBT_INT_ARRAY,"HeightMap")) == NULL || p->payload.p_int_array.size != 256)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Int Array 'HeightMap' with size 256 not found");
			return NULL;
			}
		memcpy(c->light->height,p->payload.p_int_array.data,256*4);
		//special objects
		if ((c->special->entities = nbt_child_find(t,NBT_LIST,"Entities")) == NULL || (c->special->tile_entities = nbt_child_find(t,NBT_LIST,"TileEntities")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: one or both of Lists 'Entities' & 'TileEntities' not found");
			return NULL;
			}
		c->special->tile_ticks = nbt_child_find(t,NBT_LIST,"TileTicks");
		}
	
	//read sections
	if ((t = nbt_child_find(t,NBT_LIST,"Sections")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: List 'Sections' not found");
		return NULL;
		}
	for (l = t->firstchild; l != NULL; l = l->next_sib)
		{
		
		//determine Y index
		if ((p = nbt_child_find(l,NBT_BYTE,"Y")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte 'Y' not found");
			return NULL;
			}
		//store Y index for a minute
		y_index = p->payload.p_byte;
		//block IDs
		if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"Blocks")) == NULL || p->payload.p_byte_array.size != 4096)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'Blocks' with size 4096 not found");
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'Add' did not have size 2048");
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
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'Data' with size 2048 not found");
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
		if (mode == MCMAP_FULL)
			{
			//block-emitted light
			if ((p = nbt_child_find(l,NBT_BYTE_ARRAY,"BlockLight")) == NULL || p->payload.p_byte_array.size != 2048)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'Block Light' with size 2048 not found");
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk: Byte Array 'SkyLight' with size 2048 not found");
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
		if (c->special != NULL)
			{
			c->special->entities = NULL;
			c->special->tile_entities = NULL;
			c->special->tile_ticks = NULL;
			}
		}
	
	return c;
	}

//save all existing components of the given chunk to raw NBT data; return 0 on success and -1 on failure
int _mcmap_chunk_nbt_save(struct mcmap_chunk *c)
	{
	struct nbt_tag *Level, *xPos, *zPos, *LastUpdate, *TerrainPopulated, *InhabitedTime, *Biomes, *HeightMap, *Sections, *loop, *probe, *probe1, *Entities, *TileEntities, *TileTicks;
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
			snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'Biomes' was not size 256");
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
			if (Sections->firstchild == NULL)
				Sections->payload.p_list = NBT_COMPOUND; //apparently minecraft sometimes saves empty lists with type NBT_BYTE ... ? go figure
			else
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; non-empty 'Sections' List was not type Compound");
				return -1;
				}
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Compound child of List 'Sections' did not have a 'Y' index in range");
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
					{
					nbt_free(nbt_separate(s[i]));
					s[i] = NULL;
					}
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'Blocks' was not size 4096");
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
							snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'Add' was not size 2048");
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'Data' was not size 2048");
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
				snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Int Array 'HeightMap' was not size 256");
				return -1;
				}
			}
		memcpy(HeightMap->payload.p_int_array.data,c->light->height,256*4);
		//handle sky & block light
		for (i=0;i<16;i++)
			{
			if (s[i] != NULL)
				{
				//allocate or find sky-emitted lighting data...
				if ((probe = nbt_child_find(s[i],NBT_BYTE_ARRAY,"SkyLight")) == NULL)
					{
					if ((probe = nbt_child_new(s[i],NBT_BYTE_ARRAY,"SkyLight")) == NULL)
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
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'SkyLight' was not size 2048");
						return -1;
						}
					}
				//allocate or find block-emitted lighting data...
				if ((probe1 = nbt_child_find(s[i],NBT_BYTE_ARRAY,"BlockLight")) == NULL)
					{
					if ((probe1 = nbt_child_new(s[i],NBT_BYTE_ARRAY,"BlockLight")) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
						return -1;
						}
					probe1->payload.p_byte_array.size = 2048;
					if ((probe1->payload.p_byte_array.data = (int8_t *)calloc(probe1->payload.p_byte_array.size,1)) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
						return -1;
						}
					}
				else
					{
					if (probe1->payload.p_byte_array.size != 2048)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; Byte Array 'BlockLight' was not size 2048");
						return -1;
						}
					}
				//write to lighting data...
				j=0;
				for (y = i*16; y < (i+1)*16; y++)
					{
					for (z=0;z<16;z++)
						{
						for (x=0;x<16;x++)
							{
							if (j%2 == 0)
								{
								probe->payload.p_byte_array.data[j/2] = (int8_t)(((c->light->sky[y][z][x])<<0)&0x0F);
								probe1->payload.p_byte_array.data[j/2] = (int8_t)(((c->light->block[y][z][x])<<0)&0x0F);
								}
							else
								{
								probe->payload.p_byte_array.data[j/2] = probe->payload.p_byte_array.data[j/2] | (int8_t)(((c->light->sky[y][z][x])<<4)&0xF0);
								probe1->payload.p_byte_array.data[j/2] = probe1->payload.p_byte_array.data[j/2] | (int8_t)(((c->light->block[y][z][x])<<4)&0xF0);
								}
							j++;
							}
						}
					}
				}
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
				//copy it...
				if ((Entities = nbt_copy(c->special->entities->parent)) == NULL)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
					return -1;
					}
				//find a place to put it, set structure links...
				if (Level->firstchild != NULL)
					{
					for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
					Entities->prev_sib = loop;
					loop->next_sib = Entities;
					}
				else
					Level->firstchild = Entities;
				Entities->parent = Level;
				}
			else //if the list preexists in the NBT struct...
				{
				if (Entities->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					if (Entities->firstchild == NULL)
						Entities->payload.p_list = NBT_COMPOUND; //apparently minecraft sometimes saves empty lists with type NBT_BYTE ... ? go figure
					else
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; non-empty 'Entities' List was not type Compound");
						return -1;
						}
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list... (FIXME - if the application saves more than once, it'll duplicate all the entities!)
				if (Entities != c->special->entities)
					{
					//assuming this list isn't empty...
					if (c->special->entities->firstchild != NULL)
						{
						//copy it...
						if ((probe = nbt_copy(c->special->entities)) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
							return -1;
							}
						//find a place to put the first child...
						if (Entities->firstchild != NULL)
							{
							for (loop = Entities->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
							probe->firstchild->prev_sib = loop;
							loop->next_sib = probe->firstchild;
							}
						else
							Entities->firstchild = probe->firstchild;
						//reassign the parent for all children...
						for (loop = probe->firstchild; loop != NULL; loop = loop->next_sib)
							loop->parent = Entities;
						probe->firstchild = NULL;
						nbt_free(probe);
						}
					}
				}
			}
		if (c->special->tile_entities != NULL)
			{
			if (!ishere1 || (TileEntities = nbt_child_find(Level,NBT_LIST,"TileEntities")) == NULL) //if the list is absent from the NBT struct...
				{
				//copy it...
				if ((TileEntities = nbt_copy(c->special->tile_entities)) == NULL)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
					return -1;
					}
				//find a place to put it, set structure links...
				if (Level->firstchild != NULL)
					{
					for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
					TileEntities->prev_sib = loop;
					loop->next_sib = TileEntities;
					}
				else
					Level->firstchild = TileEntities;
				TileEntities->parent = Level;
				}
			else //if the list preexists in the NBT struct...
				{
				if (TileEntities->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					if (TileEntities->firstchild == NULL)
						TileEntities->payload.p_list = NBT_COMPOUND; //apparently minecraft sometimes saves empty lists with type NBT_BYTE ... ? go figure
					else
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; non-empty 'TileEntities' List was not type Compound");
						return -1;
						}
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list... (FIXME - if the application saves more than once, it'll duplicate all the tile entities!)
				if (TileEntities != c->special->tile_entities)
					{
					//assuming this list isn't empty...
					if (c->special->tile_entities->firstchild != NULL)
						{
						//copy it...
						if ((probe = nbt_copy(c->special->tile_entities)) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
							return -1;
							}
						//find a place to put the first child...
						if (TileEntities->firstchild != NULL)
							{
							for (loop = TileEntities->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
							probe->firstchild->prev_sib = loop;
							loop->next_sib = probe->firstchild;
							}
						else
							TileEntities->firstchild = probe->firstchild;
						//reassign the parent for all children...
						for (loop = probe->firstchild; loop != NULL; loop = loop->next_sib)
							loop->parent = TileEntities;
						probe->firstchild = NULL;
						nbt_free(probe);
						}
					}
				}
			}
		if (c->special->tile_ticks != NULL)
			{
			if (!ishere1 || (TileTicks = nbt_child_find(Level,NBT_LIST,"TileTicks")) == NULL) //if the list is absent from the NBT struct...
				{
				//copy it...
				if ((TileTicks = nbt_copy(c->special->tile_ticks)) == NULL)
					{
					snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
					return -1;
					}
				//find a place to put it, set structure links...
				if (Level->firstchild != NULL)
					{
					for (loop = Level->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
					TileTicks->prev_sib = loop;
					loop->next_sib = TileTicks;
					}
				else
					Level->firstchild = TileTicks;
				TileTicks->parent = Level;
				}
			else //if the list preexists in the NBT struct...
				{
				if (TileTicks->payload.p_list != NBT_COMPOUND) //then it shouldn't be the wrong list type...
					{
					if (TileTicks->firstchild == NULL)
						TileTicks->payload.p_list = NBT_COMPOUND; //apparently minecraft sometimes saves empty lists with type NBT_BYTE ... ? go figure
					else
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"malformed chunk; non-empty 'TileTicks' List was not type Compound");
						return -1;
						}
					}
				//if we've just rediscovered the same memory space the chunk is pointing to, then to heck with it;
				//otherwise we should append it to the list, preserving both in one merged list... (FIXME - if the application saves more than once, it'll duplicate all the tile ticks!)
				if (TileTicks != c->special->tile_ticks)
					{
					//assuming this list isn't empty...
					if (c->special->tile_ticks->firstchild != NULL)
						{
						//copy it...
						if ((probe = nbt_copy(c->special->tile_ticks)) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
							return -1;
							}
						//find a place to put the first child...
						if (TileTicks->firstchild != NULL)
							{
							for (loop = TileTicks->firstchild; loop->next_sib != NULL; loop = loop->next_sib);
							probe->firstchild->prev_sib = loop;
							loop->next_sib = probe->firstchild;
							}
						else
							TileTicks->firstchild = probe->firstchild;
						//reassign the parent for all children...
						for (loop = probe->firstchild; loop != NULL; loop = loop->next_sib)
							loop->parent = TileTicks;
						probe->firstchild = NULL;
						nbt_free(probe);
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
	int s, sc, i, d, lx,lz, f, e;
	
	//save native chunk data to the raw NBT structure
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
		if (c->special != NULL)
			{
			c->special->entities = NULL;
			c->special->tile_entities = NULL;
			c->special->tile_ticks = NULL;
			}
		}
	
	//negotiate a place for the memory buffer in the 'mcmap_region' struct, and save the buffer there
	sc = (int)ceil(((double)(s+5))/4096.0);
	d  = sc - r->header->locations[z][x].sector_count;
	e  = (int)ceil(((double)r->size)/4096.0);
	r->header->locations[z][x].sector_count = (uint8_t)sc;
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
				m = (uint8_t *)r->header;
				//shuffle things starting at the end and moving backwards
				for (i = e-1; i > r->locations[z][x]; i--)
					{
					//find out which chunk this is and slide it toward the end
					f=0;
					for (lz=31; lz>=0 && !f; lz--)
						{
						for (lx=31; lx>=0 && !f; lx--)
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
					for (lz=0; lz<32 && !f; lz++)
						{
						for (lx=0; lx<32 && !f; lx++)
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
		r->locations[z][x] = e; //put it at the end
		}
	//restore all chunk pointers after potentially invalidating them with 'realloc()' or definitely invalidating them with 'memmove()'
	for (lz=0;lz<32;lz++)
		for (lx=0;lx<32;lx++)
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
		if (c->geom != NULL)
			free(c->geom);
		if (c->light != NULL)
			free(c->light);
		if (c->meta != NULL)
			free(c->meta);
		if (c->special != NULL)
			{
			if (c->special->entities != NULL)
				nbt_free(nbt_separate(c->special->entities));
			if (c->special->tile_entities != NULL)
				nbt_free(nbt_separate(c->special->tile_entities));
			if (c->special->tile_ticks != NULL)
				nbt_free(nbt_separate(c->special->tile_ticks));
			free(c->special);
			}
		if (c->raw != NULL)
			nbt_free(c->raw);
		free(c);
		}
	return;
	}

//these are convenience functions; application programmer may bypass if he knows what he's doing
//----------------------------------------------------------------------------------------------
	//retrieve data from the given world coordinates
	#define _mcmap_get_resolve(w,component1,component2,x,y,z,default) \
		{ \
		int rx,rz, cx,cz, bx,bz; \
		if ((w) == NULL || y<0 || y>=256) \
			return (default); \
		rx = (int)floor(((double)(x))/512.0) - (w)->start_x; \
		rz = (int)floor(((double)(z))/512.0) - (w)->start_z; \
		if (rx<0 || rx >= (w)->size_x || rz<0 || rz >= (w)->size_z) \
			return (default); \
		cx = (int)floor(((double)(x))/16.0); \
		cz = (int)floor(((double)(z))/16.0); \
		cx = ( (cx<0) ? ((cx+1)%32+31) : (cx%32) ); \
		cz = ( (cz<0) ? ((cz+1)%32+31) : (cz%32) ); \
		if ((w)->regions[rz][rx]->chunks[cz][cx] == NULL || (w)->regions[rz][rx]->chunks[cz][cx]->component1 == NULL) \
			return (default); \
		bx = ( ((x)<0) ? (((x)+1)%16+15) : ((x)%16) ); \
		bz = ( ((z)<0) ? (((z)+1)%16+15) : ((z)%16) ); \
		return w->regions[rz][rx]->chunks[cz][cx]->component1->component2[bz][bx]; \
		}
	uint16_t mcmap_get_block(struct mcmap_level_world *w, int x, int y, int z)
		{_mcmap_get_resolve(w,geom,blocks[y],x,y,z,MCMAP_AIR);}
	
	uint8_t mcmap_get_data(struct mcmap_level_world *w, int x, int y, int z)
		{_mcmap_get_resolve(w,geom,data[y],x,y,z,0x00);}
	
	int8_t mcmap_get_biome(struct mcmap_level_world *w, int x, int z)
		{_mcmap_get_resolve(w,geom,biomes,x,0,z,MCMAP_NOBIOME);}
	
	uint8_t mcmap_get_blocklight(struct mcmap_level_world *w, int x, int y, int z)
		{_mcmap_get_resolve(w,light,block[y],x,y,z,0x00);}
	
	uint8_t mcmap_get_skylight(struct mcmap_level_world *w, int x, int y, int z)
		{_mcmap_get_resolve(w,light,sky[y],x,y,z,0x00);}
	
	int32_t mcmap_get_heightmap(struct mcmap_level_world *w, int x, int z)
		{_mcmap_get_resolve(w,light,height,x,0,z,-1);}
	#undef _mcmap_get_resolve
	
	//edit data at the given world coordinates; return -1 if region or chunk are missing or not loaded, otherwise return 0
	#define _mcmap_set_resolve(w,component1,component2,x,y,z,value) \
		{ \
		int rx,rz, cx,cz, bx,bz; \
		if ((w) == NULL || y<0 || y>=256) \
			{ \
			snprintf(mcmap_error,MCMAP_MAXSTR,"invalid arguments"); \
			return -1; \
			} \
		rx = (int)floor(((double)(x))/512.0) - (w)->start_x; \
		rz = (int)floor(((double)(z))/512.0) - (w)->start_z; \
		if (rx<0 || rx >= (w)->size_x || rz<0 || rz >= (w)->size_z) \
			{ \
			snprintf(mcmap_error,MCMAP_MAXSTR,"region %d,%d not found", rx + (w)->start_x, rz + (w)->start_z); \
			return -1; \
			} \
		cx = (int)floor(((double)(x))/16.0); \
		cz = (int)floor(((double)(z))/16.0); \
		cx = ( (cx<0) ? ((cx+1)%32+31) : (cx%32) ); \
		cz = ( (cz<0) ? ((cz+1)%32+31) : (cz%32) ); \
		if ((w)->regions[rz][rx]->chunks[cz][cx] == NULL) \
			{ \
			snprintf(mcmap_error,MCMAP_MAXSTR,"chunk %d,%d not found in region %d,%d",cx,cz, rx + (w)->start_x, rz + (w)->start_z); \
			return -1; \
			} \
		if ((w)->regions[rz][rx]->chunks[cz][cx]->component1 == NULL) \
			{ \
			snprintf(mcmap_error,MCMAP_MAXSTR,"chunk %d,%d in region %d,%d is missing requested info",cx,cz, rx + (w)->start_x, rz + (w)->start_z); \
			return -1; \
			} \
		bx = ( ((x)<0) ? (((x)+1)%16+15) : ((x)%16) ); \
		bz = ( ((z)<0) ? (((z)+1)%16+15) : ((z)%16) ); \
		w->regions[rz][rx]->chunks[cz][cx]->component1->component2[bz][bx] = (value); \
		return 0; \
		}
	int mcmap_set_block(struct mcmap_level_world *w, int x, int y, int z, uint16_t val)
		{_mcmap_set_resolve(w,geom,blocks[y],x,y,z,val);}
	
	int mcmap_set_data(struct mcmap_level_world *w, int x, int y, int z, uint8_t val)
		{_mcmap_set_resolve(w,geom,data[y],x,y,z,val);}
	
	int mcmap_set_biome(struct mcmap_level_world *w, int x, int z, int8_t val)
		{_mcmap_set_resolve(w,geom,biomes,x,0,z,val);}
	
	int mcmap_set_blocklight(struct mcmap_level_world *w, int x, int y, int z, uint8_t val)
		{_mcmap_set_resolve(w,light,block[y],x,y,z,val);}
	
	int mcmap_set_skylight(struct mcmap_level_world *w, int x, int y, int z, uint8_t val)
		{_mcmap_set_resolve(w,light,sky[y],x,y,z,val);}
	
	int mcmap_set_heightmap(struct mcmap_level_world *w, int x, int z, int32_t val)
		{_mcmap_set_resolve(w,light,height,x,0,z,val);}
	#undef _mcmap_set_resolve
	
	//retrieve the chunk struct for the given global block coordinates
	struct mcmap_chunk *mcmap_get_chunk(struct mcmap_level_world *w, int x, int z)
		{
		int rx,rz, cx,cz;
		if ((w) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"invalid arguments");
			return NULL;
			}
		rx = (int)floor(((double)(x))/512.0) - (w)->start_x;
		rz = (int)floor(((double)(z))/512.0) - (w)->start_z;
		if (rx<0 || rx >= (w)->size_x || rz<0 || rz >= (w)->size_z)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"region %d,%d not found", rx + (w)->start_x, rz + (w)->start_z);
			return NULL;
			}
		cx = (int)floor(((double)(x))/16.0);
		cz = (int)floor(((double)(z))/16.0);
		cx = ( (cx<0) ? ((cx+1)%32+31) : (cx%32) );
		cz = ( (cz<0) ? ((cz+1)%32+31) : (cz%32) );
		if (w->regions[rz][rx]->chunks[cz][cx] == NULL)
			snprintf(mcmap_error,MCMAP_MAXSTR,"chunk index %d,%d missing from region %d,%d", cx,cz, rx + (w)->start_x, rz + (w)->start_z);
		return w->regions[rz][rx]->chunks[cz][cx];
		}

//perform lighting update on all loaded geometry in the given world of the given level, loading adjacent chunks
//when available, in order to avoid lighting seams (no need to call this function before 'mcmap_level_write()')
//return 0 on success and -1 on failure
int mcmap_light_update(struct mcmap_level *l, struct mcmap_level_world *w)
	{
	int **loaded;
	int x,y,z, lx,lz, rx,rz, bx,bz;
	int light, temp, i;
	int nr,st,ea,we;
	struct mcmap_chunk *c;
	char path[MCMAP_MAXSTR];
	
	//resolve full path from level and world components
	i = strlen(l->path);
	if (i==0 || l->path[i-1] == '/')
		snprintf(path,MCMAP_MAXSTR,"%s%s",l->path,w->path);
	else
		snprintf(path,MCMAP_MAXSTR,"%s/%s",l->path,w->path);
	
	//allocate private metadata
	if ((loaded = (int **)calloc(w->size_z*32,sizeof(int *))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return -1;
		}
	for (z=0; z < w->size_z*32; z++)
		{
		if ((loaded[z] = (int *)calloc(w->size_x*32,sizeof(int))) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return -1;
			}
		}
	
	//1st pass: prepare world & private metadata
	for (lz=0; lz < w->size_z*32; lz++)
		{
		for (lx=0; lx < w->size_x*32; lx++)
			{
			if ((c = mcmap_get_chunk(w,lx*16+w->start_x*512,lz*16+w->start_z*512)) != NULL)
				{
				loaded[lz][lx] = 1; //remember which chunks are loaded
				if (c->light == NULL) //make sure 'light' struct exists
					{
					if ((c->light = (struct mcmap_chunk_light *)calloc(1,sizeof(struct mcmap_chunk_light))) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
						return -1;
						}
					}
				mcmap_chunk_height_update(c); //update the heightmap
				for (y=0;y<256;y++) //remove all light except emitting blocks
					{
					for (z=0;z<16;z++)
						{
						for (x=0;x<16;x++)
							{
							c->light->block[y][z][x] = _mcmap_light_update_emit(c->geom->blocks[y][z][x]);
							if (y >= c->light->height[z][x])
								c->light->sky[y][z][x] = 0x0f;
							else
								c->light->sky[y][z][x] = 0x00;
							}
						}
					}
				}
			else
				loaded[lz][lx] = 0; //remember which chunks are loaded
			}
		}
	
	//2nd pass: use metadata to load adjacent chunks
	for (lz=0; lz < w->size_z*32; lz++)
		{
		for (lx=0; lx < w->size_x*32; lx++)
			{
			if (loaded[lz][lx] == 1)
				{
				nr = (lz > 0 && loaded[lz-1][lx] == 0);
				st = (lz < w->size_z*32-1 && loaded[lz+1][lx] == 0);
				ea = (lx < w->size_x*32-1 && loaded[lz][lx+1] == 0);
				we = (lx > 0 && loaded[lz][lx-1] == 0);
				if (nr) //load northward chunk
					{
					rx=lx/32; rz=(lz-1)/32;
					 x=lx%32;  z=(lz-1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz-1][lx] = 2;
					}
				if (ea) //load eastward chunk
					{
					rx=(lx+1)/32; rz=lz/32;
					 x=(lx+1)%32;  z=lz%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz][lx+1] = 2;
					}
				if (st) //load southward chunk
					{
					rx=lx/32; rz=(lz+1)/32;
					 x=lx%32;  z=(lz+1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz+1][lx] = 2;
					}
				if (we) //load westward chunk
					{
					rx=(lx-1)/32; rz=lz/32;
					 x=(lx-1)%32;  z=lz%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz][lx-1] = 2;
					}
				if (nr && ea) //load northeastward chunk
					{
					rx=(lx+1)/32; rz=(lz-1)/32;
					 x=(lx+1)%32;  z=(lz-1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz-1][lx+1] = 2;
					}
				if (st && ea) //load southeastward chunk
					{
					rx=(lx+1)/32; rz=(lz+1)/32;
					 x=(lx+1)%32;  z=(lz+1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz+1][lx+1] = 2;
					}
				if (st && we) //load southwestward chunk
					{
					rx=(lx-1)/32; rz=(lz+1)/32;
					 x=(lx-1)%32;  z=(lz+1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz+1][lx-1] = 2;
					}
				if (nr && we) //load northwestward chunk
					{
					rx=(lx-1)/32; rz=(lz-1)/32;
					 x=(lx-1)%32;  z=(lz-1)%32;
					if (w->regions[rz][rx]->raw == NULL)
						w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, path);
					if (w->regions[rz][rx]->raw != NULL)
						w->regions[rz][rx]->chunks[z][x] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[z][x]),MCMAP_FULL,1);
					loaded[lz-1][lx-1] = 2;
					}
				}
			}
		}
	
	//3rd pass: specially prepare the adjacent chunks
	for (lz=0; lz < w->size_z*32; lz++)
		{
		for (lx=0; lx < w->size_x*32; lx++)
			{
			if (loaded[lz][lx] == 2 && (c = mcmap_get_chunk(w,lx*16+w->start_x*512,lz*16+w->start_z*512)) != NULL)
				{
				mcmap_chunk_height_update(c); //update the heightmap
				//alter parameters of loop based on surrounding chunks, to preserve a single line of block light potentially emitted from unloaded chunks
				rx=16; rz=16; bx=0; bz=0;
				if (lz > 0 && loaded[lz-1][lx] == 0) //northward chunk is unloaded
					bz++;
				if (lx < w->size_x*32-1 && loaded[lz][lx+1] == 0) //eastward chunk
					rx--;
				if (lz < w->size_z*32-1 && loaded[lz+1][lx] == 0) //southward chunk
					rz--;
				if (lx > 0 && loaded[lz][lx-1] == 0) //westward chunk
					bx++;
				for (y=0;y<256;y++) //remove all light except emitting blocks
					{
					for (z=bz;z<rz;z++)
						{
						for (x=bx;x<rx;x++)
							{
							c->light->block[y][z][x] = _mcmap_light_update_emit(c->geom->blocks[y][z][x]);
							if (y >= c->light->height[z][x])
								c->light->sky[y][z][x] = 0x0f;
							else
								c->light->sky[y][z][x] = 0x00;
							}
						}
					}
				}
			}
		}
	
	//4th pass: propagate light
	for (light=15; light>0; light--) //for each light level starting at highest moving down
		{
		for (lz=0; lz < w->size_z*32; lz++) //for each chunk
			{
			for (lx=0; lx < w->size_x*32; lx++)
				{
				if ((c = mcmap_get_chunk(w,lx*16+w->start_x*512,lz*16+w->start_x*512)) != NULL) //if the chunk exists
					{
					for (y=0;y<256;y++) //for each block in the chunk
						{
						for (z=0;z<16;z++)
							{
							for (x=0;x<16;x++)
								{
								//get our global bearings
								rx = x+lx*16+w->start_x*512;
								rz = z+lz*16+w->start_z*512;
								//propagate block-emitted light
								if (c->light->block[y][z][x] == (uint8_t)light) //if the light at this block is our current pass
									{
									//examine northward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y,rz-1))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx,y,rz-1))) //only change for the brighter
										mcmap_set_blocklight(w,rx,y,rz-1,(uint8_t)temp);
									//examine eastward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx+1,y,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx+1,y,rz))) //only change for the brighter
										mcmap_set_blocklight(w,rx+1,y,rz,(uint8_t)temp);
									//examine southward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y,rz+1))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx,y,rz+1))) //only change for the brighter
										mcmap_set_blocklight(w,rx,y,rz+1,(uint8_t)temp);
									//examine westward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx-1,y,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx-1,y,rz))) //only change for the brighter
										mcmap_set_blocklight(w,rx-1,y,rz,(uint8_t)temp);
									//examine upward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y+1,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx,y+1,rz))) //only change for the brighter
										mcmap_set_blocklight(w,rx,y+1,rz,(uint8_t)temp);
									//examine downward block
									temp = ((int)c->light->block[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y-1,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_blocklight(w,rx,y-1,rz))) //only change for the brighter
										mcmap_set_blocklight(w,rx,y-1,rz,(uint8_t)temp);
									}
								//propagate sky-emitted light
								if (c->light->sky[y][z][x] == (uint8_t)light) //if the light at this block is our current pass
									{
									//examine northward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y,rz-1))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx,y,rz-1))) //only change for the brighter
										mcmap_set_skylight(w,rx,y,rz-1,(uint8_t)temp);
									//examine eastward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx+1,y,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx+1,y,rz))) //only change for the brighter
										mcmap_set_skylight(w,rx+1,y,rz,(uint8_t)temp);
									//examine southward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y,rz+1))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx,y,rz+1))) //only change for the brighter
										mcmap_set_skylight(w,rx,y,rz+1,(uint8_t)temp);
									//examine westward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx-1,y,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx-1,y,rz))) //only change for the brighter
										mcmap_set_skylight(w,rx-1,y,rz,(uint8_t)temp);
									//examine upward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y+1,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx,y+1,rz))) //only change for the brighter
										mcmap_set_skylight(w,rx,y+1,rz,(uint8_t)temp);
									//examine downward block
									temp = ((int)c->light->sky[y][z][x])-((int)_mcmap_light_update_extinct(mcmap_get_block(w,rx,y-1,rz))); //what would the propagated light level be?
									if (temp>((int)mcmap_get_skylight(w,rx,y-1,rz))) //only change for the brighter
										mcmap_set_skylight(w,rx,y-1,rz,(uint8_t)temp);
									}
								}
							}
						}
					}
				}
			}
		}
	
	//clean up
	for (z=0; z < w->size_z*32; z++)
		free(loaded[z]);
	free(loaded);
	return 0;
	}

//allocate and initialize a level struct with the given parameters, mainly including a bare-minimum 'level.dat' file;
//returns NULL on failure < http://minecraft.gamepedia.com/Level_format >
struct mcmap_level *mcmap_level_new (
	long int seed,       //random seed, will use 'time()' if given 0
	const char *name,    //level name (separate from 'path')
	const char *genname, //should be one of "default", "flat", "largeBiomes", "amplified", "customized", or "debug_all_block_states" (case insensitive)
	const char *genoptions, //may be MCMAP_OPTIONS_SUPERFLAT for default or NULL for other generators
	int structures,  //boolean flag for whether minecraft should generate structures (e.g. villages, strongholds, mineshafts)
	int gametype,    //should be one of MCMAP_GAME_SURVIVAL, MCMAP_GAME_CREATIVE, MCMAP_GAME_ADVENTURE, or MCMAP_GAME_SPECTATOR
	int hardcore,    //boolean flag for whether minecraft should delete the world upon the player's first death
	int commands,    //boolean flag for whether minecraft should allow in-game cheat commands
	int comblock,    //boolean flag for whether command blocks print to the in-game chat
	int daycyc,      //boolean flag for whether the daylight cycles
	int firetick,    //boolean flag for whether fire spreads or burns out
	int mobloot,     //boolean flag for whether mobs drop loot when killed
	int mobspawn,    //boolean flag for whether mobs spawn in the darkness
	int tiledrops,   //boolean flag for whether blocks drop anything upon breaking
	int keepinv,     //boolean flag for whether players keeps their inventories upon dying
	int mobgrief,    //boolean flag for whether mobs can destroy blocks
	int regener,     //boolean flag for whether the player's health can regenerate
	int spawnx,      //spawn coordinates (may be changed later with 'struct mcmap_level' members 'spawnx,' 'spawny,' & 'spawnz')
	int spawny,
	int spawnz,
	const char *path //path to map folder on the disk
	)
	{
	struct mcmap_level *l;
	char wpath[MCMAP_MAXSTR];
	char temp[MCMAP_MAXSTR];
	struct nbt_tag *t, *p;
	//perform sanity checks on the input
	if (seed == 0)
		seed = (long int)time(NULL);
	if (path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' is NULL");
		return NULL;
		}
	if (name == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'name' is NULL");
		return NULL;
		}
	if (genname == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'genname' is NULL");
		return NULL;
		}
	
	//allocate level...
	if ((l = (struct mcmap_level *)calloc(1,sizeof(struct mcmap_level))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//store directory paths...
	if ((l->path = (char *)calloc(strlen(path)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(l->path,path);
	snprintf(wpath,MCMAP_MAXSTR,"region/");
	if ((l->overworld.path = (char *)calloc(strlen(wpath)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(l->overworld.path,wpath);
	snprintf(wpath,MCMAP_MAXSTR,"DIM-1/");
	if ((l->nether.path = (char *)calloc(strlen(wpath)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(l->nether.path,wpath);
	snprintf(wpath,MCMAP_MAXSTR,"DIM1/");
	if ((l->end.path = (char *)calloc(strlen(wpath)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(l->end.path,wpath);
	//prepare to handle session lock later
	l->lock = ((int64_t)time(NULL))*1000;
	
	//handle 'level.dat' file
	if ((l->meta = nbt_child_new(NULL,NBT_COMPOUND,NULL)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (nbt_child_new(l->meta,NBT_COMPOUND,"Data") == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	//NBT format version
	if ((t = nbt_child_new(l->meta->firstchild,NBT_INT,"version")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_int = 19133;
	//random seed
	if ((t = nbt_child_new(l->meta->firstchild,NBT_LONG,"RandomSeed")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_long = seed;
	//level name
	if ((t = nbt_child_new(l->meta->firstchild,NBT_STRING,"LevelName")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if ((t->payload.p_string = (char *)calloc(strlen(name)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(t->payload.p_string,name);
	//handle modify timestamp
	if ((t = nbt_child_new(l->meta->firstchild,NBT_LONG,"LastPlayed")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_long = ((int64_t)time(NULL))*1000;
	//generator name
	if ((t = nbt_child_new(l->meta->firstchild,NBT_STRING,"generatorName")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if ((t->payload.p_string = (char *)calloc(strlen(genname)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(t->payload.p_string,genname);
	//generator options
	if ((t = nbt_child_new(l->meta->firstchild,NBT_STRING,"generatorOptions")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (genoptions != NULL)
		{
		if ((t->payload.p_string = (char *)calloc(strlen(genoptions)+1,1)) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
			return NULL;
			}
		strcpy(t->payload.p_string,genoptions);
		}
	//generate structures flag
	if ((t = nbt_child_new(l->meta->firstchild,NBT_BYTE,"MapFeatures")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_byte = (int8_t)structures;
	//game type
	if ((t = nbt_child_new(l->meta->firstchild,NBT_INT,"GameType")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_int = gametype;
	//hardcore flag
	if ((t = nbt_child_new(l->meta->firstchild,NBT_BYTE,"hardcore")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_byte = (int8_t)hardcore;
	//commands flag
	if ((t = nbt_child_new(l->meta->firstchild,NBT_BYTE,"allowCommands")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_byte = (int8_t)commands;
	//spawn coordinates
	if ((t = nbt_child_new(l->meta->firstchild,NBT_INT,"SpawnX")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_int = (int32_t)spawnx;
	l->spawnx = &(t->payload.p_int);
	if ((t = nbt_child_new(l->meta->firstchild,NBT_INT,"SpawnY")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_int = (int32_t)spawny;
	l->spawny = &(t->payload.p_int);
	if ((t = nbt_child_new(l->meta->firstchild,NBT_INT,"SpawnZ")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	t->payload.p_int = (int32_t)spawnz;
	l->spawnz = &(t->payload.p_int);
	//gamerules
	if ((t = nbt_child_new(l->meta->firstchild,NBT_COMPOUND,"GameRules")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	//command block flag
	if ((p = nbt_child_new(t,NBT_STRING,"commandBlockOutput")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (comblock)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//daylight cycle flag
	if ((p = nbt_child_new(t,NBT_STRING,"doDaylightCycle")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (daycyc)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//fire tick flag
	if ((p = nbt_child_new(t,NBT_STRING,"doFireTick")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (firetick)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//mob loot flag
	if ((p = nbt_child_new(t,NBT_STRING,"doMobLoot")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (mobloot)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//mob spawning flag
	if ((p = nbt_child_new(t,NBT_STRING,"doMobSpawning")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (mobspawn)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//tile drops flag
	if ((p = nbt_child_new(t,NBT_STRING,"doTileDrops")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (tiledrops)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//keep inventory flag
	if ((p = nbt_child_new(t,NBT_STRING,"keepInventory")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (keepinv)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//mob griefing flag
	if ((p = nbt_child_new(t,NBT_STRING,"mobGriefing")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (mobgrief)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	//health regeneration flag
	if ((p = nbt_child_new(t,NBT_STRING,"naturalRegeneration")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if (regener)
		snprintf(temp,MCMAP_MAXSTR,"true");
	else
		snprintf(temp,MCMAP_MAXSTR,"false");
	if ((p->payload.p_string = (char *)calloc(strlen(temp)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(p->payload.p_string,temp);
	
	return l;
	}

//worker function for 'mcmap_level_read()', called for each of 'mcmap_level's members 'overworld', 'nether', & 'end'
//returns 0 on success and -1 on failure
int _mcmap_level_world_read(struct mcmap_level *l, struct mcmap_level_world *w, const char *wpath, mcmap_mode mode, int rem)
	{
	DIR *d;
	struct dirent *e;
	int minx,minz,maxx,maxz;
	int x,z, ix,iz, lx,lz;
	int first, i;
	char fpath[MCMAP_MAXSTR];
	w->size_x = 0;
	w->size_z = 0;
	
	//resolve full path from level and world components
	i = strlen(l->path);
	if (i==0 || l->path[i-1] == '/')
		snprintf(fpath,MCMAP_MAXSTR,"%s%s",l->path,wpath);
	else
		snprintf(fpath,MCMAP_MAXSTR,"%s/%s",l->path,wpath);
	//store relative world path
	if ((w->path = (char *)calloc(strlen(wpath)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return -1;
		}
	strcpy(w->path,wpath);
	//open directory stream
	if ((d = opendir(fpath)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"opendir(\"%s\") returned NULL",fpath);
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
	
	if (!first)
		{
		//initialize world
		w->start_x = minx;
		w->start_z = minz;
		w->size_x = maxx-minx+1;
		w->size_z = maxz-minz+1;
		//allocate regions
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
		if (mode == MCMAP_FULL)
			{
			rewinddir(d);
			while ((e = readdir(d)) != NULL)
				{
				if (sscanf(e->d_name,"r.%d.%d.mca",&x,&z) == 2)
					{
					ix = x - w->start_x;
					iz = z - w->start_z;
					//read region
					if ((w->regions[iz][ix]->raw = mcmap_region_read(x,z,fpath)) != NULL)
						{
						//read chunks
						for (lz=0;lz<32;lz++)
							{
							for (lx=0;lx<32;lx++)
								w->regions[iz][ix]->chunks[lz][lx] = mcmap_chunk_read(&(w->regions[iz][ix]->raw->chunks[lz][lx]),mode,rem);
							}
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
// 'mode' should be MCMAP_PARTIAL to let the caller cherry-pick regions and chunks with
// 'mcmap_region_read()' and 'mcmap_chunk_read()', or MCMAP_FULL to read everything
// (warning: may consume LOTS of memory); 'rem' is a boolean flag for whether to remember
// the raw data at each stage; returns NULL on failure
struct mcmap_level *mcmap_level_read(const char *path, mcmap_mode mode, int rem)
	{
	struct mcmap_level *l;
	char lpath[MCMAP_MAXSTR], spath[MCMAP_MAXSTR];
	int i;
	struct nbt_tag *t;
	if (path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' is NULL");
		return NULL;
		}
	
	//resolve filename from map directory...
	i = strlen(path);
	if (i==0 || path[i-1] == '/')
		{
		snprintf(lpath,MCMAP_MAXSTR,"%slevel.dat",path);
		snprintf(spath,MCMAP_MAXSTR,"%ssession.lock",path);
		}
	else
		{
		snprintf(lpath,MCMAP_MAXSTR,"%s/level.dat",path);
		snprintf(spath,MCMAP_MAXSTR,"%s/session.lock",path);
		}
	//allocate level...
	if ((l = (struct mcmap_level *)calloc(1,sizeof(struct mcmap_level))) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//store directory path...
	if ((l->path = (char *)calloc(strlen(path)+1,1)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	strcpy(l->path,path);
	//obtain session lock
	l->lock = ((int64_t)time(NULL))*1000;
	//populate level...
	if (_mcmap_level_world_read(l,&(l->overworld),"region/",mode,rem) == -1)
		return NULL;
	if (_mcmap_level_world_read(l,&(l->nether),"DIM-1/",mode,rem) == -1)
		return NULL;
	if (_mcmap_level_world_read(l,&(l->end),"DIM1/",mode,rem) == -1)
		return NULL;
	//read 'level.dat' file...
	if ((l->meta = nbt_file_read(lpath)) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
		return NULL;
		}
	if ((t = nbt_child_find(l->meta->firstchild,NBT_INT,"SpawnX")) != NULL)
		l->spawnx = &(t->payload.p_int);
	if ((t = nbt_child_find(l->meta->firstchild,NBT_INT,"SpawnY")) != NULL)
		l->spawny = &(t->payload.p_int);
	if ((t = nbt_child_find(l->meta->firstchild,NBT_INT,"SpawnZ")) != NULL)
		l->spawnz = &(t->payload.p_int);
	
	return l;
	}

//worker function for 'mcmap_level_write()', called for each of 'mcmap_level's members 'overworld', 'nether', & 'end'
//returns 0 on success and -1 on failure
int _mcmap_level_world_write(struct mcmap_level *l, struct mcmap_level_world *w, int rem)
	{
	char fpath[MCMAP_MAXSTR];
	int i, ishere;
	int rx,rz,cx,cz;
	struct mcmap_chunk *c;
	time_t t;
	if (w->path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' string is NULL");
		return -1;
		}
	
	//resolve full path from level and world components
	i = strlen(l->path);
	if (i==0 || l->path[i-1] == '/')
		snprintf(fpath,MCMAP_MAXSTR,"%s%s",l->path,w->path);
	else
		snprintf(fpath,MCMAP_MAXSTR,"%s/%s",l->path,w->path);
	//make sure directory exists
	if (mkdir(fpath,S_IRWXU | S_IRWXG | S_IRWXO) == -1 && errno != EEXIST)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"mkdir() on '%s': %s",fpath,strerror(errno));
		return -1;
		}
	
	//loop through every region
	for (rz=0; rz < w->size_z; rz++)
		{
		for (rx=0; rx < w->size_x; rx++)
			{
			ishere = 0;
			for (cz=0;cz<32;cz++)
				{
				for (cx=0;cx<32;cx++)
					{
					if ((c = w->regions[rz][rx]->chunks[cz][cx]) != NULL) //okay we have a loaded chunk now
						{
						ishere = 1;
						//make sure the region is loaded
						if (w->regions[rz][rx]->raw == NULL)
							{
							//first try to read it from the disk
							if ((w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, fpath)) == NULL)
								{
								//failing that, create it anew
								if ((w->regions[rz][rx]->raw = mcmap_region_new()) == NULL)
									return -1;
								}
							}
						//update the timestamp
						if ((t = time(NULL)) != -1)
							w->regions[rz][rx]->raw->dates[cz][cx] = t;
						//save the chunk
						if (mcmap_chunk_write(w->regions[rz][rx]->raw,cx,cz,c,rem) == -1)
							return -1;
						}
					}
				}
			//save the region
			if (ishere)
				{
				if (mcmap_region_write(w->regions[rz][rx]->raw, rx + w->start_x, rz + w->start_z, fpath) == -1)
					return -1;
				}
			//clean up
			if (!rem && w->regions[rz][rx]->raw != NULL)
				{
				mcmap_region_free(w->regions[rz][rx]->raw);
				w->regions[rz][rx]->raw = NULL;
				}
			}
		}
	
	return 0;
	}

//write to the disk all loaded chunks in the given level using its member 'path'; rem is a boolean flag
//for whether to remember the raw data afterward; returns 0 on success and -1 on failure
int mcmap_level_write(struct mcmap_level *l, int rem)
	{
	char lpath[MCMAP_MAXSTR], spath[MCMAP_MAXSTR];
	int64_t i;
	int64_t ret;
	uint8_t b[8];
	FILE *f;
	struct nbt_tag *t;
	if (l == NULL)
		return 0;
	if (l->path == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"'path' string is NULL");
		return -1;
		}
	//make sure directory exists
	if (mkdir(l->path,S_IRWXU | S_IRWXG | S_IRWXO) == -1 && errno != EEXIST)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"mkdir() on '%s': %s",l->path,strerror(errno));
		return -1;
		}
	//resolve filenames from map directory...
	i = strlen(l->path);
	if (i==0 || l->path[i-1] == '/')
		{
		snprintf(lpath,MCMAP_MAXSTR,"%slevel.dat",l->path);
		snprintf(spath,MCMAP_MAXSTR,"%ssession.lock",l->path);
		}
	else
		{
		snprintf(lpath,MCMAP_MAXSTR,"%s/level.dat",l->path);
		snprintf(spath,MCMAP_MAXSTR,"%s/session.lock",l->path);
		}
	
	//avoid lighting glitches from changes to the geometry, since removing it only CRASHES minecraft instead of forcing a lighting update in-game
	if (mcmap_light_update(l,&(l->overworld)) == -1)
		return -1;
	if (mcmap_light_update(l,&(l->nether)) == -1)
		return -1;
	if (mcmap_light_update(l,&(l->end)) == -1)
		return -1;
	//check if we still have the session lock...
	if ((f = fopen(spath,"rb+")) == NULL)
		{
		//if the file doesn't exist, create it...
		cswapw_64(b,l->lock);
		if ((f = fopen(spath,"wb")) == NULL)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on '%s': %s",spath,strerror(errno));
			return -1;
			}
		if (fwrite(b,1,8,f) != 8)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"fwrite() returned short item count on '%s'",spath);
			return -1;
			}
		}
	else
		{
		//make sure the lock file doesn't show activity after our level struct was created...
		if ((i = fread(b,1,8,f)) != 8)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"fread() encountered %s on the last %d requested bytes of '%s'",(ferror(f)?"an error":"EOF"),8-(int)i,spath);
			return -1;
			}
		if ((ret = cswapr_64(b)) > l->lock)
			{
			i = ret - l->lock;
			snprintf(mcmap_error,MCMAP_MAXSTR,"session lock was taken by another application after %ld millisecond%s",(long)i,(i==1?"":"s"));
			return -1;
			}
		//we're okay, now record our activity to alert other applications...
		if (fseek(f,0,SEEK_SET) == -1)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"fseek() on '%s': %s",spath,strerror(errno));
			return -1;
			}
		l->lock = time(NULL)*1000;
		cswapw_64(b,l->lock);
		if (fwrite(b,1,8,f) != 8)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"fwrite() returned short item count on '%s'",spath);
			return -1;
			}
		}
	fclose(f);
	//save chunks...
	if (_mcmap_level_world_write(l,&(l->overworld),rem) == -1)
		return -1;
	if (_mcmap_level_world_write(l,&(l->nether),rem) == -1)
		return -1;
	if (_mcmap_level_world_write(l,&(l->end),rem) == -1)
		return -1;
	//write 'level.dat' file...
	if (l->meta != NULL)
		{
		//update timestamp...
		if ((t = nbt_child_find(l->meta->firstchild,NBT_LONG,"LastPlayed")) != NULL)
			t->payload.p_long = (int64_t)time(NULL)*1000;
		//write file...
		if (nbt_file_write(lpath,l->meta,NBT_COMPRESS_GZIP) == -1)
			{
			snprintf(mcmap_error,MCMAP_MAXSTR,"%s: %s",NBT_LIBNAME,nbt_error);
			return -1;
			}
		}
	
	return 0;
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
			if (w->regions[z][x] != NULL)
				{
				for (lz=0;lz<32;lz++)
					{
					for (lx=0;lx<32;lx++)
						{
						if (w->regions[z][x]->chunks[lz][lx] != NULL)
							mcmap_chunk_free(w->regions[z][x]->chunks[lz][lx]);
						}
					}
				if (w->regions[z][x]->raw != NULL)
					mcmap_region_free(w->regions[z][x]->raw);
				free(w->regions[z][x]);
				}
			}
		free(w->regions[z]);
		}
	free(w->regions);
	free(w->path);
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
	if (l->path != NULL)
		free(l->path);
	free(l);
	return;
	}

//make sure the chunk & region at the given global block coordinates are loaded if they exist; 'create' is a boolean
// flag for whether to create them if they do not already exist; 'rem' is a flag for whether to remember raw data
// after reading (for faster resaving); 'mode' being MCMAP_PARTIAL saves memory in simple read-only use cases, but
// MCMAP_FULL is reasonable for this function; returns 0 on success and -1 on failure
int mcmap_prime_single(struct mcmap_level *l, struct mcmap_level_world *w, int x, int z, mcmap_mode mode, int rem, int create)
	{
	int i, lx,lz;
	int rx,rz, gcx,gcz, cx,cz;
	char fpath[MCMAP_MAXSTR];
	if (w == NULL || l == NULL)
		return 0;
	
	//resolve full path from level and world components
	i = strlen(l->path);
	if (i==0 || l->path[i-1] == '/')
		snprintf(fpath,MCMAP_MAXSTR,"%s%s",l->path,w->path);
	else
		snprintf(fpath,MCMAP_MAXSTR,"%s/%s",l->path,w->path);
	//make sure region array exists, resolve region & chunk indices
	if (w->regions == NULL)
		{
		if (create)
			{
			w->size_x = 1;
			w->size_z = 1;
			if ((w->regions       = (struct mcmap_level_region ***)calloc(1,sizeof(struct mcmap_level_region **))) == NULL ||
			    (w->regions[0]    = (struct mcmap_level_region  **)calloc(1,sizeof(struct mcmap_level_region  *))) == NULL ||
			    (w->regions[0][0] = (struct mcmap_level_region   *)calloc(1,sizeof(struct mcmap_level_region   ))) == NULL)
				{
				snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
				return -1;
				}
			w->start_x = (int)floor(((double)x)/512.0);
			w->start_z = (int)floor(((double)z)/512.0);
			rx = 0;
			rz = 0;
			}
		else
			return 0;
		}
	else
		{
		rx = (int)floor(((double)x)/512.0) - w->start_x;
		rz = (int)floor(((double)z)/512.0) - w->start_z;
		//make sure we have room if we're planning to create the region
		if (rx<0 || rx >= w->size_x || rz<0 || rz >= w->size_z)
			{
			if (create)
				{
				if (rz >= w->size_z)
					{
					//remember old size for a sec
					i = w->size_z;
					//expand array
					w->size_z = rz+1;
					if ((w->regions = (struct mcmap_level_region ***)realloc(w->regions,w->size_z*sizeof(struct mcmap_level_region **))) == NULL)
						{
						snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
						return -1;
						}
					//fill newly allocated space
					for (lz=i; lz < w->size_z; lz++)
						{
						if ((w->regions[lz] = (struct mcmap_level_region **)calloc(w->size_x,sizeof(struct mcmap_level_region *))) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
							return -1;
							}
						for (lx=0; lx < w->size_x; lx++)
							{
							if ((w->regions[lz][lx] = (struct mcmap_level_region *)calloc(1,sizeof(struct mcmap_level_region))) == NULL)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
								return -1;
								}
							}
						}
					}
				else
					{
					if (rz < 0)
						{
						//expand array
						w->size_z -= rz;
						w->start_z += rz;
						if ((w->regions = (struct mcmap_level_region ***)realloc(w->regions,w->size_z*sizeof(struct mcmap_level_region **))) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
							return -1;
							}
						//slide all the old entries back
						for (lz = w->size_z-1; lz >= -rz; lz--)
							w->regions[lz] = w->regions[lz+rz];
						//fill newly allocated space
						for (lz=0; lz < -rz; lz++)
							{
							if ((w->regions[lz] = (struct mcmap_level_region **)calloc(w->size_x,sizeof(struct mcmap_level_region *))) == NULL)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
								return -1;
								}
							for (lx=0; lx < w->size_x; lx++)
								{
								if ((w->regions[lz][lx] = (struct mcmap_level_region *)calloc(1,sizeof(struct mcmap_level_region))) == NULL)
									{
									snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
									return -1;
									}
								}
							}
						//update the index
						rz = 0;
						}
					}
				if (rx >= w->size_x)
					{
					//remember old size for a sec
					i = w->size_x;
					//expand array
					w->size_x = rx+1;
					for (lz=0; lz < w->size_z; lz++)
						{
						if ((w->regions[lz] = (struct mcmap_level_region **)realloc(w->regions[lz],w->size_x*sizeof(struct mcmap_level_region *))) == NULL)
							{
							snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
							return -1;
							}
						//fill newly allocated space
						for (lx=i; lx < w->size_x; lx++)
							{
							if ((w->regions[lz][lx] = (struct mcmap_level_region *)calloc(1,sizeof(struct mcmap_level_region))) == NULL)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
								return -1;
								}
							}
						}
					}
				else
					{
					if (rx < 0)
						{
						//expand array
						w->size_x -= rx;
						w->start_x += rx;
						for (lz=0; lz < w->size_z; lz++)
							{
							if ((w->regions[lz] = (struct mcmap_level_region **)realloc(w->regions[lz],w->size_x*sizeof(struct mcmap_level_region *))) == NULL)
								{
								snprintf(mcmap_error,MCMAP_MAXSTR,"realloc() returned NULL");
								return -1;
								}
							//slide all the old entries back
							for (lx = w->size_x-1; lx >= -rx; lx--)
								w->regions[lz][lx] = w->regions[lz][lx+rx];
							//fill newly allocated space
							for (lx=0; lx < -rx; lx++)
								{
								if ((w->regions[lz][lx] = (struct mcmap_level_region *)calloc(1,sizeof(struct mcmap_level_region))) == NULL)
									{
									snprintf(mcmap_error,MCMAP_MAXSTR,"calloc() returned NULL");
									return -1;
									}
								}
							}
						//update the index
						rx = 0;
						}
					}
				}
			else
				return 0;
			}
		}
	gcx = (int)floor(((double)x)/16.0);
	gcz = (int)floor(((double)z)/16.0);
	cx = ( (gcx<0) ? ((gcx+1)%32+31) : (gcx%32) );
	cz = ( (gcz<0) ? ((gcz+1)%32+31) : (gcz%32) );
	//load chunk
	if (w->regions[rz][rx]->chunks[cz][cx] == NULL)
		{
		//make sure the region is loaded
		if (w->regions[rz][rx]->raw == NULL)
			{
			if ((w->regions[rz][rx]->raw = mcmap_region_read(rx + w->start_x, rz + w->start_z, fpath)) == NULL)
				{
				if (create)
					{
					if ((w->regions[rz][rx]->raw = mcmap_region_new()) == NULL)
						return -1;
					}
				else
					return 0;
				}
			}
		//try to load the chunk
		if ((w->regions[rz][rx]->chunks[cz][cx] = mcmap_chunk_read(&(w->regions[rz][rx]->raw->chunks[cz][cx]),mode,rem)) == NULL && create)
			{
			if ((w->regions[rz][rx]->chunks[cz][cx] = mcmap_chunk_new(gcx,gcz,mode)) == NULL)
				return -1;
			}
		}
	return 0;
	}

//call 'mcmap_prime_single()' for all blocks in the rectangle between the two given coordinate pairs, inclusive; return 0 on success and -1 on failure
int mcmap_prime_rect(struct mcmap_level *l, struct mcmap_level_world *w, int x1, int z1, int x2, int z2, mcmap_mode mode, int rem, int create)
	{
	int x,z, cx1,cz1, cx2,cz2;
	//locate which corner is which
	if (x2 < x1)
		{
		x = x2;
		x2 = x1;
		x1 = x;
		}
	if (z2 < z1)
		{
		z = z2;
		z2 = z1;
		z1 = z;
		}
	//resolve chunk coordinates for the appropriate corners
	cx1 = (int)floor(((double)x1)/16.0);
	cz1 = (int)floor(((double)z1)/16.0);
	cx2 = (int)floor(((double)x2)/16.0);
	cz2 = (int)floor(((double)z2)/16.0);
	//loop through chunks
	for (z=cz1; z<=cz2; z++)
		{
		for (x=cx1; x<=cx2; x++)
			{
			if (mcmap_prime_single(l,w,x*16,z*16,mode,rem,create) == -1)
				return -1;
			}
		}
	return 0;
	}

//call 'mcmap_prime_single()' for all blocks in the circle with the given center and radius; return 0 on success and -1 on failure
int mcmap_prime_circle(struct mcmap_level *l, struct mcmap_level_world *w, int x, int z, double radius, mcmap_mode mode, int rem, int create)
	{
	int bx,bz, cx1,cz1, cx2,cz2, cx,cz;
	//locate corners
	cx1 = (int)floor(floor(((double)x)-radius)/16.0);
	cz1 = (int)floor(floor(((double)z)-radius)/16.0);
	cx2 = (int)floor( ceil(((double)x)+radius)/16.0);
	cz2 = (int)floor( ceil(((double)z)+radius)/16.0);
	//loop through chunks
	for (cz=cz1; cz<=cz2; cz++)
		{
		for (cx=cx1; cx<=cx2; cx++)
			{
			//find the block in the chunk closest to the center of the circle to test
			bx = cx*16;
			if (bx < x)
				{
				bx += 15;
				if (bx > x)
					bx = x;
				}
			bz = cz*16;
			if (bz < z)
				{
				bz += 15;
				if (bz > z)
					bz = z;
				}
			//load the chunk if its closest corner intrudes in the circle
			if (sqrt(pow((double)(bx-x),2)+pow((double)(bz-z),2)) <= radius && mcmap_prime_single(l,w,bx,bz,mode,rem,create) == -1)
				return -1;
			}
		}
	return 0;
	}
