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

#ifndef __MCMAP_HEADER
#define __MCMAP_HEADER

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "libnbt/nbt.h"

#define MCMAP_LIBNAME "libmcmap"
#define MCMAP_MAXSTR 2048

char mcmap_error[MCMAP_MAXSTR]; //in error conditions, this will be populated with a detailed human-readable message


//general definitions
//-------------------
//(scroll past for overview documentation)

//minecraft 1.6.2 data values < http://www.minecraftwiki.net/wiki/Data_values >
typedef enum
	{
	MCMAP_AIR                       =   0,
	MCMAP_STONE                     =   1,
	MCMAP_GRASS                     =   2,
	MCMAP_DIRT                      =   3,
	MCMAP_COBBLESTONE               =   4,
	MCMAP_WOOD_PLANKS               =   5,
	MCMAP_SAPLING                   =   6,
	MCMAP_BEDROCK                   =   7,
	MCMAP_FLOWING_WATER             =   8,
	MCMAP_WATER                     =   9,
	MCMAP_FLOWING_LAVA              =  10,
	MCMAP_LAVA                      =  11,
	MCMAP_SAND                      =  12,
	MCMAP_GRAVEL                    =  13,
	MCMAP_GOLD_ORE                  =  14,
	MCMAP_IRON_ORE                  =  15,
	MCMAP_COAL_ORE                  =  16,
	MCMAP_WOOD                      =  17,
	MCMAP_LEAVES                    =  18,
	MCMAP_SPONGE                    =  19,
	MCMAP_GLASS                     =  20,
	MCMAP_LAPIS_ORE                 =  21,
	MCMAP_LAPIS_BLOCK               =  22,
	MCMAP_DISPENSER                 =  23,
	MCMAP_SANDSTONE                 =  24,
	MCMAP_NOTE_BLOCK                =  25,
	MCMAP_BED                       =  26,
	MCMAP_POWERED_RAIL              =  27,
	MCMAP_DETECTOR_RAIL             =  28,
	MCMAP_STICKY_PISTON             =  29,
	MCMAP_COBWEB                    =  30,
	MCMAP_TALL_GRASS                =  31,
	MCMAP_DEAD_BUSH                 =  32,
	MCMAP_PISTON                    =  33,
	MCMAP_PISTON_EXT                =  34,
	MCMAP_WOOL                      =  35,
	MCMAP_MOVED_BY_PISTON           =  36,
	MCMAP_DANDELION                 =  37,
	MCMAP_ROSE                      =  38,
	MCMAP_BROWN_MUSHROOM            =  39,
	MCMAP_RED_MUSHROOM              =  40,
	MCMAP_GOLD_BLOCK                =  41,
	MCMAP_IRON_BLOCK                =  42,
	MCMAP_DOUBLE_SLAB               =  43,
	MCMAP_SLAB                      =  44,
	MCMAP_BRICKS                    =  45,
	MCMAP_TNT                       =  46,
	MCMAP_BOOKSHELF                 =  47,
	MCMAP_MOSS_STONE                =  48,
	MCMAP_OBSIDIAN                  =  49,
	MCMAP_TORCH                     =  50,
	MCMAP_FIRE                      =  51,
	MCMAP_SPAWNER                   =  52,
	MCMAP_OAK_WOOD_STAIRS           =  53,
	MCMAP_CHEST                     =  54,
	MCMAP_REDSTONE_WIRE             =  55,
	MCMAP_DIAMOND_ORE               =  56,
	MCMAP_DIAMOND_BLOCK             =  57,
	MCMAP_CRAFTING_TABLE            =  58,
	MCMAP_WHEAT                     =  59,
	MCMAP_FARMLAND                  =  60,
	MCMAP_FURNACE                   =  61,
	MCMAP_LIT_FURNACE               =  62,
	MCMAP_SIGN                      =  63,
	MCMAP_WOOD_DOOR                 =  64,
	MCMAP_LADDER                    =  65,
	MCMAP_RAIL                      =  66,
	MCMAP_COBBLESTONE_STAIRS        =  67,
	MCMAP_WALL_SIGN                 =  68,
	MCMAP_LEVER                     =  69,
	MCMAP_STONE_PRESSURE_PLATE      =  70,
	MCMAP_IRON_DOOR                 =  71,
	MCMAP_WOOD_PRESSURE_PLATE       =  72,
	MCMAP_REDSTONE_ORE              =  73,
	MCMAP_GLOWING_REDSTONE_ORE      =  74,
	MCMAP_UNLIT_REDSTONE_TORCH      =  75,
	MCMAP_LIT_REDSTONE_TORCH        =  76,
	MCMAP_STONE_BUTTON              =  77,
	MCMAP_SNOW                      =  78,
	MCMAP_ICE                       =  79,
	MCMAP_SNOW_BLOCK                =  80,
	MCMAP_CACTUS                    =  81,
	MCMAP_CLAY_BLOCK                =  82,
	MCMAP_SUGAR_CANE                =  83,
	MCMAP_JUKEBOX                   =  84,
	MCMAP_FENCE                     =  85,
	MCMAP_PUMPKIN                   =  86,
	MCMAP_NETHERRACK                =  87,
	MCMAP_SOUL_SAND                 =  88,
	MCMAP_GLOWSTONE                 =  89,
	MCMAP_NETHER_PORTAL             =  90,
	MCMAP_JACK_O_LANTERN            =  91,
	MCMAP_PLACED_CAKE               =  92,
	MCMAP_UNLIT_REDSTONE_REPEATER   =  93,
	MCMAP_LIT_REDSTONE_REPEATER     =  94,
	MCMAP_LOCKED_CHEST              =  95,
	MCMAP_TRAPDOOR                  =  96,
	MCMAP_SILVERFISH_EGG            =  97,
	MCMAP_STONE_BRICKS              =  98,
	MCMAP_HUGE_BROWN_MUSHROOM       =  99,
	MCMAP_HUGE_RED_MUCHROOM         = 100,
	MCMAP_IRON_BARS                 = 101,
	MCMAP_GLASS_PANE                = 102,
	MCMAP_MELON                     = 103,
	MCMAP_PUMPKIN_STEM              = 104,
	MCMAP_MELON_STEM                = 105,
	MCMAP_VINES                     = 106,
	MCMAP_GATE                      = 107,
	MCMAP_BRICK_STAIRS              = 108,
	MCMAP_STONE_BRICK_STAIRS        = 109,
	MCMAP_MYCELIUM                  = 110,
	MCMAP_LILY_PAD                  = 111,
	MCMAP_NETHER_BRICK              = 112,
	MCMAP_NETHER_BRICK_FENCE        = 113,
	MCMAP_NETHER_BRICK_STAIRS       = 114,
	MCMAP_NETHER_WART               = 115,
	MCMAP_ENCHANTING_TABLE          = 116,
	MCMAP_BREWING_STAND             = 117,
	MCMAP_CAULDRON                  = 118,
	MCMAP_END_PORTAL                = 119,
	MCMAP_END_PORTAL_FRAME          = 120,
	MCMAP_END_STONE                 = 121,
	MCMAP_DRAGON_EGG                = 122,
	MCMAP_UNLIT_REDSTONE_LAMP       = 123,
	MCMAP_LIT_REDSTONE_LAMP         = 124,
	MCMAP_WOOD_DOUBLE_SLAB          = 125,
	MCMAP_WOOD_SLAB                 = 126,
	MCMAP_COCOA_POD                 = 127,
	MCMAP_SANDSTONE_STAIRS          = 128,
	MCMAP_EMERALD_ORE               = 129,
	MCMAP_ENDER_CHEST               = 130,
	MCMAP_TRIPWIRE_HOOK             = 131,
	MCMAP_TRIPWIRE                  = 132,
	MCMAP_EMERALD_BLOCK             = 133,
	MCMAP_SPRUCE_WOOD_STAIRS        = 134,
	MCMAP_BIRCH_WOOD_STAIRS         = 135,
	MCMAP_JUNGLE_WOOD_STAIRS        = 136,
	MCMAP_COMMAND_BLOCK             = 137,
	MCMAP_BEACON                    = 138,
	MCMAP_COBBLESTONE_WALL          = 139,
	MCMAP_FLOWER_POT                = 140,
	MCMAP_CARROTS                   = 141,
	MCMAP_POTATOES                  = 142,
	MCMAP_WOOD_BUTTON               = 143,
	MCMAP_HEAD                      = 144,
	MCMAP_ANVIL                     = 145,
	MCMAP_TRAPPED_CHEST             = 146,
	MCMAP_LIGHT_PRESSURE_PLATE      = 147,
	MCMAP_HEAVY_PRESSURE_PLATE      = 148,
	MCMAP_UNLIT_REDSTONE_CAMPARATOR = 149,
	MCMAP_LIT_REDSTONE_COMPARATOR   = 150,
	MCMAP_DAYLIGHT_SENSOR           = 151,
	MCMAP_REDSTONE_BLOCK            = 152,
	MCMAP_NETHER_QUARTZ_ORE         = 153,
	MCMAP_HOPPER                    = 154,
	MCMAP_QUARTZ_BLOCK              = 155,
	MCMAP_QUARTZ_STAIRS             = 156,
	MCMAP_ACTIVATOR_RAIL            = 157,
	MCMAP_DROPPER                   = 158,
	MCMAP_STAINED_CLAY              = 159,
	                               // 160,
	                               // 161,
	                               // 162,
	                               // 163,
	                               // 164,
	                               // 165,
	                               // 166,
	                               // 167,
	                               // 168,
	                               // 169,
	MCMAP_HAY_BLOCK                 = 170,
	MCMAP_CARPET                    = 171,
	MCMAP_HARDENED_CLAY             = 172,
	MCMAP_COAL_BLOCK                = 173
	} mcmap_blockid;
typedef enum
	{
	MCMAP_NOBIOME             = -1,
	MCMAP_OCEAN               =  0,
	MCMAP_PLAINS              =  1,
	MCMAP_DESERT              =  2,
	MCMAP_EXTREME_HILLS       =  3,
	MCMAP_FOREST              =  4,
	MCMAP_TAIGA               =  5,
	MCMAP_SWAMPLAND           =  6,
	MCMAP_RIVER               =  7,
	MCMAP_HELL                =  8,
	MCMAP_SKY                 =  9,
	MCMAP_FROZENOCEAN         = 10,
	MCMAP_FROZENRIVER         = 11,
	MCMAP_ICE_PLAINS          = 12,
	MCMAP_ICE_MOUNTAINS       = 13,
	MCMAP_MUSHROOMISLAND      = 14,
	MCMAP_MUSHROOMISLANDSHORE = 15,
	MCMAP_BEACH               = 16,
	MCMAP_DESERTHILLS         = 17,
	MCMAP_FORESTHILLS         = 18,
	MCMAP_TAIGAHILLS          = 19,
	MCMAP_EXTREME_HILLS_EDGE  = 20,
	MCMAP_JUNGLE              = 21,
	MCMAP_JUNGLEHILLS         = 22
	} mcmap_biomeid;


// map data scheme, to be read in 5 stages:
// ----------------------------------------
//		1) anvil (.rca) region format < http://www.minecraftwiki.net/wiki/Region_file_format >
//	--extract--
//		2) individual compressed chunk
//	--uncompress--
//		3) NBT format < http://www.minecraftwiki.net/wiki/NBT_format >
//	--parse--
//		4) NBT-based memory structure < http://www.minecraftwiki.net/wiki/Chunk_format >
//	--parse--
//		5) minecraft-based memory structure


// stage 1: region file in memory
// ------------------------------

//bit-for-bit structured copy of file metadata
struct __attribute__((__packed__)) mcmap_region_chunk_location //location atom for a given chunk
	{
	uint8_t offset[3]; //number of 4KiB sectors between start of file and start of chunk
	uint8_t sector_count; //size of chunk in 4KiB sectors
	};
struct __attribute__((__packed__)) mcmap_region_header //8KiB file header
	{
	struct mcmap_region_chunk_location locations[32][32]; //chunk (x,z)'s location atom is at locations[z][x] when x & z are positive; location[31-z][31-x] when negative
	uint8_t dates[32][32][4]; //UNIX timestamps for when each chunk was last modified
	};
struct __attribute__((__packed__)) mcmap_region_chunk_header //5-byte metadata for each chunk
	{
	uint8_t length[4]; //size of chunk in bytes, starting with the 1-byte compression type flag here in the metadata
	uint8_t compression; //compression type flag; 0x01 for GZip (RFC1952, unused in practice) and 0x02 for Zlib (RFC1950)
	};

//nodes to dynamically navigate file contents
struct mcmap_region_chunk
	{
	uint8_t separate; //boolean flag, 1 if 'header' & 'data' point to memory allocated individually and in need of freeing individually (MUST REMAIN ACCURATE TO AVOID LEAKS & SEGFAULTS)
	struct mcmap_region_chunk_header *header; //to point at 5-byte chunk metadata
	size_t size; //parsed copy of big-endian 32-bit integer at header->length, with 1 subtracted for compression flag
	uint8_t *data; //to point at the next byte
	};
struct mcmap_region
	{
	struct mcmap_region_header *header; //to point at a new RAM copy of the file header
	time_t dates[32][32]; //parsed copies of 32-bit integers at header->dates[z][x]
	uint32_t locations[32][32]; //parsed copies of 24-bit integers at header->locations[z][x].offset
	struct mcmap_region_chunk chunks[32][32]; //per-chunk navigation nodes, also in [Z][X] order
	};

//searches the given path to a minecraft map folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; if error returns NULL
struct mcmap_region *mcmap_region_read(int, int, const char *);

//free all memory allocated in 'mcmap_region_read()' or 'mcmap_region_new()'
void mcmap_region_free(struct mcmap_region *);


// -------------------------------------
//  --stages 2-4 are handled by libnbt--
// -------------------------------------


// stage 5: native minecraft memory structure
// ------------------------------------------

//read mode for 'mcmap_chunk_read()'
typedef enum
	{
	MCMAP_READ_PARTIAL, //load only the 'geom' member of 'struct mcmap_chunk' to save memory
	MCMAP_READ_FULL //load entire 'struct mcmap_chunk' from raw NBT data
	} mcmap_readmode;

//geometry
struct mcmap_chunk_geom
	{
	uint16_t blocks[256][16][16]; //file specification says this can be 8-bit or 12-bit
	uint8_t    data[256][16][16]; //block metadata; these are always 4-bit
	uint8_t  biomes     [16][16]; //these are 8-bit
	};
//lighting
struct mcmap_chunk_light
	{
	uint8_t  block[256][16][16]; //4-bit block-emitted light level
	uint8_t    sky[256][16][16]; //4-bit sky-emitted light level
	int32_t height     [16][16]; //32-bit lowest Y value in each column where sky-emitted light is full strength
	};
//chunk metadata
struct mcmap_chunk_meta
	{
	int64_t mtime; //in-game tick value for when the chunk was saved
	int8_t populated; //boolean flag for whether minecraft generated special features in this terrain
	int64_t itime; //cumulative number of player-ticks (like man-hours) that have occurred in this block, used for growing the difficulty
	};
//special objects (entities, tile entities, and tile ticks)
//FIXME - fully implement this rather than point to raw NBT data
struct mcmap_chunk_special
	{
	struct nbt_tag *entities;
	struct nbt_tag *tile_entities;
	struct nbt_tag *tile_ticks;
	};

//containing structure, with NULL-able pointers to save memory
struct mcmap_chunk
	{
	struct mcmap_chunk_geom    *geom;
	struct mcmap_chunk_light   *light;
	struct mcmap_chunk_meta    *meta;
	struct mcmap_chunk_special *special;

	int32_t x,z; //per-region chunk coordinates (should be >= 0, and < 32)
	
	struct nbt_tag *raw; //optionally retain stage-4 NBT structure
	};

//takes an individual chunk from a 'struct mcmap_region,' returns a parsed 'mcmap_chunk;'
//'mode' should be MCMAP_READ_FULL for fully populated chunk, MCMAP_READ_PARTIAL to save memory
//on simple geometry inquiries; 'rem' is a boolean flag for whether to remember the raw NBT structure; returns NULL on error
struct mcmap_chunk *mcmap_chunk_read(struct mcmap_region_chunk *, mcmap_readmode mode, int rem);

//save all existing components of the given chunk to the given coords in the given region, return 0 on success and -1 on failure
int mcmap_chunk_write(struct mcmap_region *, int x, int z, struct mcmap_chunk *);

//free all memory allocated in 'mcmap_chunk_read()' or 'mcmap_chunk_new()'
void mcmap_chunk_free(struct mcmap_chunk *);

#endif
