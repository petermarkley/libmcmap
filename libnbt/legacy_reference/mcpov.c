/* compile with: 'cc -Wall -o mcpov ./mcpov.c ./mcmap/mcmap.c libnbt.a -lz' */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>
//#include <sys/stat.h>

#include "./mcmap/mcmap.h"

#define TEX_PATH "terrain.png"
#define TEX_PATH_ARID "terrain_tint_arid.png"
#define TEX_PATH_COLD "terrain_tint_cold.png"
#define TEX_PATH_STMY "terrain_tint_steamy.png"

#define ARG_INPUT_SHORT        "-i"
#define ARG_INPUT_LONG    "--input"
#define ARG_OUTPUT_SHORT       "-o"
#define ARG_OUTPUT_LONG  "--output"
#define ARG_HELP_SHORT         "-h"
#define ARG_HELP_LONG      "--help"
#define ARG_SIMPLE_SHORT       "-s"
#define ARG_SIMPLE_LONG  "--simple"
#define ARG_X "-x"
#define ARG_Z "-z"

//mesh macros ftw
#define pov_upface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0)

#define pov_downface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_frontface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_backface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0)

#define pov_rightface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_leftface(x,y,z,tex) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n", \
  tex, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0)

#define pov_tint_upface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0)

#define pov_tint_downface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_tint_frontface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_tint_backface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0)

#define pov_tint_rightface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0)

#define pov_tint_leftface(x,y,z,flta,texa,fltb,texb,fltc,texc) \
  fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n", \
  flta,texa,fltb,texb,fltc,texc, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0)

//variable names for povray scene
#define MCPOV_STONE        "TEX_STONE"
#define MCPOV_GRASS_A_ARID "TEX_GRASS1_ARID"
#define MCPOV_GRASS_A_COLD "TEX_GRASS1_COLD"
#define MCPOV_GRASS_A_STMY "TEX_GRASS1_STMY"
#define MCPOV_GRASS_B_ARID "TEX_GRASS2_ARID"
#define MCPOV_GRASS_B_COLD "TEX_GRASS2_COLD"
#define MCPOV_GRASS_B_STMY "TEX_GRASS2_STMY"
#define MCPOV_GRASS_C      "TEX_GRASS3"
#define MCPOV_DIRT         "TEX_DIRT"

#define MCPOV_SAND     "TEX_SAND"
#define MCPOV_GRAVEL   "TEX_GRAVEL"
#define MCPOV_GOLD_ORE "TEX_GOLD_ORE"
#define MCPOV_IRON_ORE "TEX_IRON_ORE"
#define MCPOV_COAL_ORE "TEX_COAL_ORE"

#define MCPOV_WOOD_A "TEX_WOOD1"
#define MCPOV_WOOD_B "TEX_WOOD2"
#define MCPOV_WOOD_C "TEX_WOOD3"
#define MCPOV_WOOD_D "TEX_WOOD4"
#define MCPOV_WOOD_E "TEX_WOOD5"

#define MCPOV_LEAVES_A_ARID "TEX_LEAVES1_ARID"
#define MCPOV_LEAVES_A_COLD "TEX_LEAVES1_COLD"
#define MCPOV_LEAVES_A_STMY "TEX_LEAVES1_STMY"
#define MCPOV_LEAVES_A_GREY "TEX_LEAVES1_GREY"
#define MCPOV_LEAVES_B_ARID "TEX_LEAVES2_ARID"
#define MCPOV_LEAVES_B_COLD "TEX_LEAVES2_COLD"
#define MCPOV_LEAVES_B_STMY "TEX_LEAVES2_STMY"
#define MCPOV_LEAVES_C_ARID "TEX_LEAVES3_ARID"
#define MCPOV_LEAVES_C_COLD "TEX_LEAVES3_COLD"
#define MCPOV_LEAVES_C_STMY "TEX_LEAVES3_STMY"

#define MCPOV_DEAD_SHRUB      "TEX_DEAD_SHRUB"
#define MCPOV_TALL_GRASS_ARID "TEX_TALL_GRASS_ARID"
#define MCPOV_TALL_GRASS_COLD "TEX_TALL_GRASS_COLD"
#define MCPOV_TALL_GRASS_STMY "TEX_TALL_GRASS_STMY"
#define MCPOV_FERN_ARID       "TEX_FERN_ARID"
#define MCPOV_FERN_COLD       "TEX_FERN_COLD"
#define MCPOV_FERN_STMY       "TEX_FERN_STMY"
#define MCPOV_DANDELION       "TEX_DANDELION"
#define MCPOV_ROSE            "TEX_ROSE"
#define MCPOV_BROWN_MUSHROOM  "TEX_BROWN_MUSHROOM"
#define MCPOV_RED_MUSHROOM    "TEX_RED_MUSHROOM"

#define MCPOV_SNOW         "TEX_SNOW"
#define MCPOV_CLAY         "TEX_CLAY"
#define MCPOV_VINES_ARID   "TEX_VINES_ARID"
#define MCPOV_VINES_COLD   "TEX_VINES_COLD"
#define MCPOV_VINES_STMY   "TEX_VINES_STMY"
#define MCPOV_NONE         "TEX_NONE"

#define MCPOV_WATER_BLOCK "WATER_BLOCK"

//color tint mode
typedef enum
	{
	MCPOV_ARID = 0,
	MCPOV_COLD = 1,
	MCPOV_STMY = 2
	} _colortint_mode;

//global variables
struct mcpov_region
	{
	struct mcmap_chunk *(chunks[32][32]);
	} **regions;
int min_x, min_z, max_x, max_z;

//differentiate opaque from transparent blocks
int _is_opaque(mcmap_blockid id)
	{
	switch (id)
		{
		case MCMAP_AIR:
		case MCMAP_ICE:
		case MCMAP_GLASS:
		case MCMAP_LEAVES:
		case MCMAP_STICKY_PISTON:
		case MCMAP_PISTON:
		case MCMAP_PISTON_EXT:
		//case MCMAP_GLOWSTONE:
		//case MCMAP_UNLIT_REDSTONE_LAMP:
		//case MCMAP_LIT_REDSTONE_LAMP:
		case MCMAP_CHEST:
		case MCMAP_ENDER_CHEST:
		case MCMAP_BEACON:
		case MCMAP_MOVED_BY_PISTON:
		
		case MCMAP_FARMLAND:
		case MCMAP_SLAB:
		case MCMAP_WOOD_SLAB:
		case MCMAP_OAK_WOOD_STAIRS:
		case MCMAP_COBBLESTONE_STAIRS:
		case MCMAP_BRICK_STAIRS:
		case MCMAP_STONE_BRICK_STAIRS:
		case MCMAP_SANDSTONE_STAIRS:
		case MCMAP_SPRUCE_WOOD_STAIRS:
		case MCMAP_BIRCH_WOOD_STAIRS:
		case MCMAP_JUNGLE_WOOD_STAIRS:
		case MCMAP_NETHER_BRICK_STAIRS:
		case MCMAP_LADDER:
		case MCMAP_FENCE:
		case MCMAP_GATE:
		case MCMAP_NETHER_BRICK_FENCE:
		case MCMAP_COBBLESTONE_WALL:
		case MCMAP_PLACED_CAKE:
		case MCMAP_BED:
		case MCMAP_WOOD_DOOR:
		case MCMAP_IRON_DOOR:
		case MCMAP_UNLIT_REDSTONE_REPEATER:
		case MCMAP_LIT_REDSTONE_REPEATER:
		case MCMAP_TRAPDOOR:
		case MCMAP_COBWEB:
		case MCMAP_FLOWER_POT:
		case MCMAP_IRON_BARS:
		case MCMAP_GLASS_PANE:
		
		case MCMAP_RAIL:
		case MCMAP_POWERED_RAIL:
		case MCMAP_DETECTOR_RAIL:
		case MCMAP_LEVER:
		case MCMAP_STONE_PRESSURE_PLATE:
		case MCMAP_WOOD_PRESSURE_PLATE:
		case MCMAP_STONE_BUTTON:
		case MCMAP_WOOD_BUTTON:
		case MCMAP_REDSTONE_WIRE:
		case MCMAP_UNLIT_REDSTONE_TORCH:
		case MCMAP_LIT_REDSTONE_TORCH:
		case MCMAP_ENCHANTING_TABLE:
		case MCMAP_SNOW:
		case MCMAP_TORCH:
		case MCMAP_SIGN:
		case MCMAP_WALL_SIGN:
		case MCMAP_FIRE:
		case MCMAP_NETHER_PORTAL:
		case MCMAP_END_PORTAL:
		case MCMAP_BREWING_STAND:
		case MCMAP_CAULDRON:
		case MCMAP_END_PORTAL_FRAME:
		case MCMAP_DRAGON_EGG:
		case MCMAP_TRIPWIRE_HOOK:
		case MCMAP_TRIPWIRE:
		case MCMAP_HEAD:
		case MCMAP_ANVIL:
		case MCMAP_SPAWNER:
		
		case MCMAP_CACTUS:
		case MCMAP_SUGAR_CANE:
		case MCMAP_WHEAT:
		case MCMAP_PUMPKIN_STEM:
		case MCMAP_MELON_STEM:
		case MCMAP_NETHER_WART:
		case MCMAP_CARROTS:
		case MCMAP_POTATOES:
		case MCMAP_DANDELION:
		case MCMAP_ROSE:
		case MCMAP_BROWN_MUSHROOM:
		case MCMAP_RED_MUSHROOM:
		case MCMAP_SAPLING:
		case MCMAP_TALL_GRASS:
		case MCMAP_DEAD_BUSH:
		case MCMAP_VINES:
		case MCMAP_LILY_PAD:
		case MCMAP_COCOA_POD:
		
		case MCMAP_FLOWING_WATER:
		case MCMAP_WATER:
		case MCMAP_FLOWING_LAVA:
		case MCMAP_LAVA:
		return 0; break;
		
		default:
		return 1; break;
		}
	return -1;
	}

int _can_vineover(mcmap_blockid id)
	{
	switch (id)
		{
		case MCMAP_AIR:
		case MCMAP_SAPLING:
		case MCMAP_FLOWING_WATER:
		case MCMAP_WATER:
		case MCMAP_FLOWING_LAVA:
		case MCMAP_LAVA:
		case MCMAP_BED:
		case MCMAP_POWERED_RAIL:
		case MCMAP_DETECTOR_RAIL:
		case MCMAP_STICKY_PISTON:
		case MCMAP_COBWEB:
		case MCMAP_TALL_GRASS:
		case MCMAP_DEAD_BUSH:
		case MCMAP_PISTON:
		case MCMAP_PISTON_EXT:
		case MCMAP_MOVED_BY_PISTON:
		case MCMAP_DANDELION:
		case MCMAP_ROSE:
		case MCMAP_BROWN_MUSHROOM:
		case MCMAP_RED_MUSHROOM:
		case MCMAP_SLAB:
		case MCMAP_TORCH:
		case MCMAP_FIRE:
		case MCMAP_SPAWNER:
		case MCMAP_OAK_WOOD_STAIRS:
		case MCMAP_CHEST:
		case MCMAP_REDSTONE_WIRE:
		case MCMAP_WHEAT:
		case MCMAP_SIGN:
		case MCMAP_WOOD_DOOR:
		case MCMAP_LADDER:
		case MCMAP_RAIL:
		case MCMAP_COBBLESTONE_STAIRS:
		case MCMAP_WALL_SIGN:
		case MCMAP_LEVER:
		case MCMAP_STONE_PRESSURE_PLATE:
		case MCMAP_IRON_DOOR:
		case MCMAP_WOOD_PRESSURE_PLATE:
		case MCMAP_UNLIT_REDSTONE_TORCH:
		case MCMAP_LIT_REDSTONE_TORCH:
		case MCMAP_STONE_BUTTON:
		case MCMAP_SNOW:
		case MCMAP_CACTUS:
		case MCMAP_SUGAR_CANE:
		case MCMAP_FENCE:
		case MCMAP_NETHER_PORTAL:
		case MCMAP_PLACED_CAKE:
		case MCMAP_UNLIT_REDSTONE_REPEATER:
		case MCMAP_LIT_REDSTONE_REPEATER:
		case MCMAP_TRAPDOOR:
		case MCMAP_IRON_BARS:
		case MCMAP_GLASS_PANE:
		case MCMAP_PUMPKIN_STEM:
		case MCMAP_MELON_STEM:
		case MCMAP_VINES:
		case MCMAP_GATE:
		case MCMAP_BRICK_STAIRS:
		case MCMAP_STONE_BRICK_STAIRS:
		case MCMAP_LILY_PAD:
		case MCMAP_NETHER_BRICK_FENCE:
		case MCMAP_NETHER_BRICK_STAIRS:
		case MCMAP_NETHER_WART:
		case MCMAP_BREWING_STAND:
		case MCMAP_CAULDRON:
		case MCMAP_END_PORTAL:
		case MCMAP_DRAGON_EGG:
		case MCMAP_WOOD_SLAB:
		case MCMAP_COCOA_POD:
		case MCMAP_SANDSTONE_STAIRS:
		case MCMAP_ENDER_CHEST:
		case MCMAP_TRIPWIRE_HOOK:
		case MCMAP_TRIPWIRE:
		case MCMAP_SPRUCE_WOOD_STAIRS:
		case MCMAP_BIRCH_WOOD_STAIRS:
		case MCMAP_JUNGLE_WOOD_STAIRS:
		case MCMAP_COBBLESTONE_WALL:
		case MCMAP_FLOWER_POT:
		case MCMAP_CARROTS:
		case MCMAP_POTATOES:
		case MCMAP_WOOD_BUTTON:
		case MCMAP_HEAD:
		case MCMAP_ANVIL:
		return 0; break;
		
		default:
		return 1; break;
		}
	return -1;
	}

//return weight for povray average function based on a pixel location in the color triangle (minecraft.jar/misc/grasscolor.png)
float _colortint(int pix_x, int pix_y, _colortint_mode mode)
	{
	switch (mode)
		{
		case MCPOV_ARID:
			return sqrt(2*pow((255.0-(float)pix_x-(float)pix_y)/2.0,2))/180.312;
		break;
		case MCPOV_COLD:
			return (float)pix_x/255.0;
		break;
		case MCPOV_STMY:
			return (float)pix_y/255.0;
		break;
		}
	return 0.0;
	}

//return pixel location for a biome (dimension 0 = x, 1 = y)
int _biometint(mcmap_biomeid biome, int dim)
	{
	switch (biome)
		{
		case MCMAP_OCEAN: return (!dim?127:64); break;
		case MCMAP_RIVER:
		case MCMAP_BEACH:
		case MCMAP_PLAINS: return (!dim?50:82); break;
		case MCMAP_DESERTHILLS:
		case MCMAP_DESERT: return 0; break;
		case MCMAP_EXTREME_HILLS_EDGE:
		case MCMAP_EXTREME_HILLS: return (!dim?203:16); break;
		case MCMAP_FORESTHILLS:
		case MCMAP_FOREST: return (!dim?76:143); break;
		case MCMAP_FROZENOCEAN:
		case MCMAP_FROZENRIVER:
		case MCMAP_ICE_PLAINS:
		case MCMAP_ICE_MOUNTAINS:
		case MCMAP_TAIGAHILLS:
		case MCMAP_TAIGA: return (!dim?255:0); break;
		case MCMAP_SWAMPLAND: return (!dim?50:184); break;
		case MCMAP_MUSHROOMISLANDSHORE:
		case MCMAP_MUSHROOMISLAND: return (!dim?25:230); break;
		case MCMAP_JUNGLEHILLS:
		case MCMAP_JUNGLE: return (!dim?0:255); break;
		default: return (!dim?128:127); break;
		}
	return -1;
	}

void _print_help()
	{
	fprintf(stderr,"Created by Peter Markley (Phos_Quartz).\n\n %s %s\tPath to a Minecraft map folder ('<PATH>/minecraft/saves/<MY WORLD>' http://minecraft.net/).\n %s %s\tPath to output, a POV-Ray include file ('*.inc' http://povray.org/).\n %s MIN MAX\tRange of x coordinates for conversion area.\n %s MIN MAX\tRange of z coordinates for conversion area.\n %s %s\tDisplay this help.\n %s %s\tMake geometry simple (for faster rendering).\n",ARG_INPUT_SHORT,ARG_INPUT_LONG,ARG_OUTPUT_SHORT,ARG_OUTPUT_LONG,ARG_X,ARG_Z,ARG_HELP_SHORT,ARG_HELP_LONG,ARG_SIMPLE_SHORT,ARG_SIMPLE_LONG);
	return;
	}

int _get_region(int b)
	{
	int r = b/512;
	if (b < 0)
		r -= 1;
	return r;
	}

mcmap_blockid _get_block(int x, int y, int z)
	{
	int rx=_get_region(x), rz=_get_region(z),
	    cx=(x/16)%32, cz=(z/16)%32,
	    bx=x%16, bz=z%16;
	if (x < min_x || x > max_x || y < 0 || y > 255 || z < min_z || z > max_z)
		return MCMAP_AIR;
	
	if (x < 0)
		{
		cx += 31;
		bx += 15;
		}
	if (z < 0)
		{
		cz += 31;
		bz += 15;
		}
	
	if (regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx] == NULL)
		return MCMAP_AIR;
	return regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx]->blocks[y][bz][bx];
	}

mcmap_biomeid _get_biome(int x, int z)
	{
	int rx=_get_region(x), rz=_get_region(z),
	    cx=(x/16)%32, cz=(z/16)%32,
	    bx=x%16, bz=z%16;
	if (x < min_x || x > max_x || z < min_z || z > max_z)
		return MCMAP_NOBIOME;
	
	if (x < 0)
		{
		cx += 31;
		bx += 15;
		}
	if (z < 0)
		{
		cz += 31;
		bz += 15;
		}
	
	if (regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx] == NULL)
		return MCMAP_NOBIOME;
	return regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx]->biomes[bz][bx];
	}

unsigned char _get_data(int x, int y, int z)
	{
	int rx=_get_region(x), rz=_get_region(z),
	    cx=(x/16)%32, cz=(z/16)%32,
	    bx=x%16, bz=z%16;
	if (x < min_x || x > max_x || y < 0 || y > 255 || z < min_z || z > max_z)
		return 0x00;
	
	if (x < 0)
		{
		cx += 31;
		bx += 15;
		}
	if (z < 0)
		{
		cz += 31;
		bz += 15;
		}
	
	if (regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx] == NULL)
		return 0x00;
	return regions[rz-_get_region(min_z)][rx-_get_region(min_x)].chunks[cz][cx]->data[y][bz][bx];
	}

int main(int argc, char **argv)
	{
	FILE *scene;
	struct mcmap_region *reg;
	char *input = NULL, *output = NULL;
	int y,z,x, rx,rz, cx,cz, pix_x,pix_y, i, foundx = 0, foundz = 0, simple = 0;
	float s;
	
	//parse arguments
	if (argc == 1)
		{
		_print_help();
		return 0;
		}
	for (i=1; i<argc; i++)
		{
		if ((strcmp(argv[i],ARG_INPUT_SHORT) == 0) || (strcmp(argv[i],ARG_INPUT_LONG) == 0))
			{
			if (i == argc)
				{
				fprintf(stderr,"mcpov error: too few arguments\n");
				return -1;
				}
			input = argv[++i];
			}
		else if ((strcmp(argv[i],ARG_OUTPUT_SHORT) == 0) || (strcmp(argv[i],ARG_OUTPUT_LONG) == 0))
			{
			if (i == argc)
				{
				fprintf(stderr,"mcpov error: too few arguments\n");
				return -1;
				}
			output = argv[++i];
			}
		else if ((strcmp(argv[i],ARG_HELP_SHORT) == 0) || (strcmp(argv[i],ARG_HELP_LONG) == 0))
			{
			_print_help();
			return 0;
			}
		else if (strcmp(argv[i],ARG_X) == 0)
			{
			if (i+1 >= argc)
				{
				fprintf(stderr,"mcpov error: too few arguments\n");
				return -1;
				}
			if (sscanf(argv[++i],"%d",&min_x) != 1)
				{
				fprintf(stderr,"mcpov error: could not read MIN x value '%s'\n",argv[i]);
				return -1;
				}
			if (sscanf(argv[++i],"%d",&max_x) != 1)
				{
				fprintf(stderr,"mcpov error: could not read MAX x value '%s'\n",argv[i]);
				return -1;
				}
			if (max_x < min_x)
				{
				x = max_x;
				max_x = min_x;
				min_x = x;
				}
			foundx = 1;
			}
		else if (strcmp(argv[i],ARG_Z) == 0)
			{
			if (i+1 >= argc)
				{
				fprintf(stderr,"mcpov error: too few arguments\n");
				return -1;
				}
			if (sscanf(argv[++i],"%d",&min_z) != 1)
				{
				fprintf(stderr,"mcpov error: could not read MIN z value '%s'\n",argv[i]);
				return -1;
				}
			if (sscanf(argv[++i],"%d",&max_z) != 1)
				{
				fprintf(stderr,"mcpov error: could not read MAX z value '%s'\n",argv[i]);
				return -1;
				}
			if (max_z < min_z)
				{
				z = max_z;
				max_z = min_z;
				min_z = z;
				}
			foundz = 1;
			}
		else if (strcmp(argv[i],ARG_SIMPLE_SHORT) == 0 || strcmp(argv[i],ARG_SIMPLE_LONG) == 0)
			simple = 1;
		else
			{
			fprintf(stderr,"mcpov error: unknown argument '%s'\n",argv[i]);
			return -1;
			}
		}
	if (input == NULL)
		{
		fprintf(stderr,"mcpov error: missing input directory\n");
		return -1;
		}
	if (output == NULL)
		{
		fprintf(stderr,"mcpov error: missing output file\n");
		return -1;
		}
	if (!foundx)
		{
		fprintf(stderr,"mcpov error: missing x coordinate range\n");
		return -1;
		}
	if (!foundz)
		{
		fprintf(stderr,"mcpov error: missing z coordinate range\n");
		return -1;
		}
	
	//initialize map
	if ((regions = (struct mcpov_region **)calloc(_get_region(max_z)-_get_region(min_z)+1,sizeof(struct mcpov_region *))) == NULL)
		{
		fprintf(stderr,"mcpov error: out of memory\n");
		return -1;
		}
	for (rz=0; rz<=_get_region(max_z)-_get_region(min_z); rz++)
		{
		if ((regions[rz] = (struct mcpov_region *)calloc(_get_region(max_x)-_get_region(min_x)+1,sizeof(struct mcpov_region))) == NULL)
			{
			fprintf(stderr,"mcpov error: out of memory\n");
			return -1;
			}
		for (rx=0; rx<=_get_region(max_x)-_get_region(min_x); rx++)
			{
			reg = mcmap_read_region(rx+_get_region(min_x),rz+_get_region(min_z),input);
			if (reg != NULL)
				{
				for (cz=0; cz<32; cz++)
					{
					for (cx=0; cx<32; cx++)
						regions[rz][rx].chunks[cz][cx] = mcmap_read_chunk(cx,cz,reg);
					}
				mcmap_free_region(reg);
				}
			}
		}
	
	//print scene
	if ((scene = fopen(output,"w")) == NULL)
		{
		fprintf(stderr,"mcpov error %d: could not open '%s' for writing\n",errno,output);
		return 1;
		}
	if (simple)
		{
		fprintf(scene,"#declare %s = texture {pigment {rgb           125/255}};\n",MCPOV_STONE);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 99, 94, 24>/255}};\n",MCPOV_GRASS_A_ARID);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 73,102, 86>/255}};\n",MCPOV_GRASS_A_COLD);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 15,109,  0>/255}};\n",MCPOV_GRASS_A_STMY);
		fprintf(scene,"#declare %s = texture {pigment {rgb <136, 98, 69>/255}};\n",MCPOV_GRASS_B_ARID);
		fprintf(scene,"#declare %s = texture {pigment {rgb <136, 98, 69>/255}};\n",MCPOV_GRASS_B_COLD);
		fprintf(scene,"#declare %s = texture {pigment {rgb <136, 98, 69>/255}};\n",MCPOV_GRASS_B_STMY);
		fprintf(scene,"#declare %s = texture {pigment {rgb <136, 98, 69>/255}};\n",MCPOV_GRASS_C);
		fprintf(scene,"#declare %s = texture {pigment {rgb <136, 98, 69>/255}};\n",MCPOV_DIRT);
		fprintf(scene,"#declare %s = texture {pigment {rgb <219,212,160>/255}};\n",MCPOV_SAND);
		fprintf(scene,"#declare %s = texture {pigment {rgb <128,125,123>/255}};\n",MCPOV_GRAVEL);
		fprintf(scene,"#declare %s = texture {pigment {rgb <252,238, 75>/255}};\n",MCPOV_GOLD_ORE);
		fprintf(scene,"#declare %s = texture {pigment {rgb <216,175,147>/255}};\n",MCPOV_IRON_ORE);
		fprintf(scene,"#declare %s = texture {pigment {rgb            55/255}};\n",MCPOV_COAL_ORE);
		fprintf(scene,"#declare %s = texture {pigment {rgb <176,143, 88>/255}};\n",MCPOV_WOOD_A);
		fprintf(scene,"#declare %s = texture {pigment {rgb <101, 80, 50>/255}};\n",MCPOV_WOOD_B);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 45, 28, 12>/255}};\n",MCPOV_WOOD_C);
		fprintf(scene,"#declare %s = texture {pigment {rgb <226,225,219>/255}};\n",MCPOV_WOOD_D);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 88, 69, 27>/255}};\n",MCPOV_WOOD_E);
		fprintf(scene,"#declare %s = texture {pigment {rgb <119,113, 29>/255}};\n",MCPOV_LEAVES_A_ARID);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 91,128,107>/255}};\n",MCPOV_LEAVES_A_COLD);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 18,136,  0>/255}};\n",MCPOV_LEAVES_A_STMY);
		fprintf(scene,"#declare %s = texture {pigment {rgb           181/255}};\n",MCPOV_LEAVES_A_GREY);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 84, 80, 20>/255}};\n",MCPOV_LEAVES_B_ARID);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 62, 87, 73>/255}};\n",MCPOV_LEAVES_B_COLD);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 13, 92,  0>/255}};\n",MCPOV_LEAVES_B_STMY);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 88, 82, 21>/255}};\n",MCPOV_LEAVES_C_ARID);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 65, 90, 74>/255}};\n",MCPOV_LEAVES_C_COLD);
		fprintf(scene,"#declare %s = texture {pigment {rgb < 13, 96,  0>/255}};\n",MCPOV_LEAVES_C_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.2,0>}};\n",MCPOV_DEAD_SHRUB,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.3,-1.5,0>}};\n",MCPOV_DANDELION,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.2,-1.5,0>}};\n",MCPOV_ROSE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.3,-1.4,0>}};\n",MCPOV_BROWN_MUSHROOM,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.2,-1.4,0>}};\n",MCPOV_RED_MUSHROOM,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {rgb <243,252,252>/255}};\n",MCPOV_SNOW);
		fprintf(scene,"#declare %s = texture {pigment {rgb <159,165,177>/255}};\n",MCPOV_CLAY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {rgb <214,127,255>/255}};\n",MCPOV_NONE);
		}
	else
		{
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.1,-1.5,0>}};\n",MCPOV_STONE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <0,-1.5,0>}};\n", MCPOV_GRASS_A_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <0,-1.5,0>}};\n", MCPOV_GRASS_A_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <0,-1.5,0>}};\n", MCPOV_GRASS_A_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.3,-1.5,0>}} texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.6,-1.3,0>}};\n", MCPOV_GRASS_B_ARID,TEX_PATH,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.3,-1.5,0>}} texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.6,-1.3,0>}};\n", MCPOV_GRASS_B_COLD,TEX_PATH,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.3,-1.5,0>}} texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.6,-1.3,0>}};\n", MCPOV_GRASS_B_STMY,TEX_PATH,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.1,0>}};\n", MCPOV_GRASS_C,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.2,-1.5,0>}};\n",MCPOV_DIRT,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.2,-1.4,0>}};\n",MCPOV_SAND,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.3,-1.4,0>}};\n",MCPOV_GRAVEL,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <0,-1.3,0>}};\n",MCPOV_GOLD_ORE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.1,-1.3,0>}};\n",MCPOV_IRON_ORE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.2,-1.3,0>}};\n",MCPOV_COAL_ORE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.5,-1.4,0>}};\n",MCPOV_WOOD_A,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.4,0>}};\n",MCPOV_WOOD_B,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.8,0>}};\n",MCPOV_WOOD_C,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.5,-0.8,0>}};\n",MCPOV_WOOD_D,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.9,-0.6,0>}};\n",MCPOV_WOOD_E,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.2,0>}};\n",MCPOV_LEAVES_A_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.2,0>}};\n",MCPOV_LEAVES_A_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.2,0>}};\n",MCPOV_LEAVES_A_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-1.2,0>}};\n",MCPOV_LEAVES_A_GREY,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.7,0>}};\n",MCPOV_LEAVES_B_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.7,0>}};\n",MCPOV_LEAVES_B_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.7,0>}};\n",MCPOV_LEAVES_B_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.3,0>}};\n",MCPOV_LEAVES_C_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.3,0>}};\n",MCPOV_LEAVES_C_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.4,-0.3,0>}};\n",MCPOV_LEAVES_C_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.2,0>}};\n",MCPOV_DEAD_SHRUB,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.7,-1.3,0>}};\n",MCPOV_TALL_GRASS_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.2,0>}};\n",MCPOV_FERN_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.3,-1.5,0>}};\n",MCPOV_DANDELION,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.2,-1.5,0>}};\n",MCPOV_ROSE,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.3,-1.4,0>}};\n",MCPOV_BROWN_MUSHROOM,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.2,-1.4,0>}};\n",MCPOV_RED_MUSHROOM,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.2,-1.1,0>}};\n",MCPOV_SNOW,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.8,-1.1,0>}};\n",MCPOV_CLAY,TEX_PATH);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_ARID,TEX_PATH_ARID);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_COLD,TEX_PATH_COLD);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-1.5,-0.7,0>}};\n",MCPOV_VINES_STMY,TEX_PATH_STMY);
		fprintf(scene,"#declare %s = texture {pigment {image_map {png \"%s\"} scale 1.6 translate <-0.9,-0.1,0>}};\n",MCPOV_NONE,TEX_PATH);
		}
	fprintf(scene,"\n");
	for (y=0; y<256; y++)
		{
		for (z=min_z; z<=max_z; z++)
			{
			for (x=min_x; x<=max_x; x++)
				{
				pix_x = (int)(((float)_biometint(_get_biome(x-1,z-1),0) + (float)_biometint(_get_biome(x,z-1),0) + (float)_biometint(_get_biome(x+1,z-1),0) +
				               (float)_biometint(_get_biome(x-1,z+0),0) + (float)_biometint(_get_biome(x,z+0),0) + (float)_biometint(_get_biome(x+1,z+0),0) + 
				               (float)_biometint(_get_biome(x-1,z+1),0) + (float)_biometint(_get_biome(x,z+1),0) + (float)_biometint(_get_biome(x+1,z+1),0))/9.0);
				pix_y = (int)(((float)_biometint(_get_biome(x-1,z-1),1) + (float)_biometint(_get_biome(x,z-1),1) + (float)_biometint(_get_biome(x+1,z-1),1) +
				               (float)_biometint(_get_biome(x-1,z+0),1) + (float)_biometint(_get_biome(x,z+0),1) + (float)_biometint(_get_biome(x+1,z+0),1) + 
				               (float)_biometint(_get_biome(x-1,z+1),1) + (float)_biometint(_get_biome(x,z+1),1) + (float)_biometint(_get_biome(x+1,z+1),1))/9.0);
				switch (_get_block(x,y,z))
					{
					case MCMAP_AIR:
					break;
					case MCMAP_STONE:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_STONE);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_STONE);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_STONE);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_STONE);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_STONE);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_STONE);
					break;
					case MCMAP_GRASS:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_tint_upface   (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_GRASS_A_ARID,
							                    _colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_GRASS_A_COLD,
							                    _colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_GRASS_A_STMY);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface      (x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x,y,z+1)))
							{
							if (y != 255 && (_get_block(x,y+1,z) == MCMAP_SNOW || _get_block(x,y+1,z) == MCMAP_SNOW_BLOCK))
								pov_frontface     (x,y,z,MCPOV_GRASS_C);
							else
								pov_tint_frontface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_GRASS_B_ARID,
													_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_GRASS_B_COLD,
													_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_GRASS_B_STMY);
							}
						if (!_is_opaque(_get_block(x,y,z-1)))
							{
							if (y != 255 && (_get_block(x,y+1,z) == MCMAP_SNOW || _get_block(x,y+1,z) == MCMAP_SNOW_BLOCK))
								pov_backface      (x,y,z,MCPOV_GRASS_C);
							else
								pov_tint_backface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_GRASS_B_ARID,
													_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_GRASS_B_COLD,
													_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_GRASS_B_STMY);
							}
						if (!_is_opaque(_get_block(x+1,y,z)))
							{
							if (y != 255 && (_get_block(x,y+1,z) == MCMAP_SNOW || _get_block(x,y+1,z) == MCMAP_SNOW_BLOCK))
								pov_rightface     (x,y,z,MCPOV_GRASS_C);
							else
								pov_tint_rightface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_GRASS_B_ARID,
													_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_GRASS_B_COLD,
													_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_GRASS_B_STMY);
							}
						if (!_is_opaque(_get_block(x-1,y,z)))
							{
							if (y != 255 && (_get_block(x,y+1,z) == MCMAP_SNOW || _get_block(x,y+1,z) == MCMAP_SNOW_BLOCK))
								pov_leftface      (x,y,z,MCPOV_GRASS_C);
							else
								pov_tint_leftface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_GRASS_B_ARID,
													_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_GRASS_B_COLD,
													_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_GRASS_B_STMY);
							}
					break;
					case MCMAP_DIRT:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_DIRT);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_DIRT);
					break;
					case MCMAP_FLOWING_WATER:
					case MCMAP_WATER:
						if (!_is_opaque(_get_block(x,y+1,z)) && _get_block(x,y+1,z) != MCMAP_WATER && _get_block(x,y+1,z) != MCMAP_FLOWING_WATER)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} interior {ior 1.33} rotate <90,0,0> translate <%.1f,%.1f,%.1f> texture {pigment {rgbf <0.4,0.75,1,1>} finish {reflection {0,1}} normal {bozo scale 0.05}}}\n", (float)x/10.0,(float)(y+1)/10.0-0.2/16.0,(float)((-z)-1)/10.0);
					break;
					case MCMAP_SAND:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_SAND);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_SAND);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_SAND);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_SAND);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_SAND);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_SAND);
					break;
					case MCMAP_GRAVEL:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_GRAVEL);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_GRAVEL);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_GRAVEL);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_GRAVEL);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_GRAVEL);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_GRAVEL);
					break;
					case MCMAP_GOLD_ORE:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_GOLD_ORE);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_GOLD_ORE);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_GOLD_ORE);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_GOLD_ORE);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_GOLD_ORE);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_GOLD_ORE);
					break;
					case MCMAP_IRON_ORE:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_IRON_ORE);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_IRON_ORE);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_IRON_ORE);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_IRON_ORE);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_IRON_ORE);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_IRON_ORE);
					break;
					case MCMAP_COAL_ORE:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_COAL_ORE);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_COAL_ORE);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_COAL_ORE);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_COAL_ORE);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_COAL_ORE);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_COAL_ORE);
					break;
					case MCMAP_WOOD:
						if (!_is_opaque(_get_block(x,y+1,z)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x0)
								pov_upface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									}
							}
						if (!_is_opaque(_get_block(x,y-1,z)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x0)
								pov_downface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x8)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									}
							}
						if (!_is_opaque(_get_block(x,y,z+1)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x8)
								pov_frontface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,-90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,-90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,-90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,-90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
									break;
									}
							}
						if (!_is_opaque(_get_block(x,y,z-1)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x8)
								pov_backface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,0,90> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
									break;
									}
							}
						if (!_is_opaque(_get_block(x+1,y,z)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x4)
								pov_rightface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
									break;
									}
							}
						if (!_is_opaque(_get_block(x-1,y,z)))
							{
							if ((_get_data(x,y,z)&0xC) == 0x4)
								pov_leftface(x,y,z,MCPOV_WOOD_A);
							else
								switch (_get_data(x,y,z)&0x3)
									{
									default:
									case 0x0:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <-90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_B, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x1:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <-90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_C, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x2:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <-90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_D, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									case 0x3:
										if ((_get_data(x,y,z)&0xC) == 0x0)
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
										else
											fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> rotate <-90,0,0> translate <%.1f,%.1f,%.1f>}\n",
											MCPOV_WOOD_E, (float)x/10.0,(float)(y+1)/10.0,(float)(-z)/10.0);
									break;
									}
							}
					break;
					case MCMAP_LEAVES:
						switch (_get_data(x,y,z)&0x3)
							{
							default:
							case 0x0:
								if (!_is_opaque(_get_block(x,y+1,z)))
									pov_tint_upface   (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
								if (!_is_opaque(_get_block(x,y-1,z)))
									pov_tint_downface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
								if (!_is_opaque(_get_block(x,y,z+1)))
									pov_tint_frontface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
								if (!_is_opaque(_get_block(x,y,z-1)))
									pov_tint_backface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
								if (!_is_opaque(_get_block(x+1,y,z)))
									pov_tint_rightface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
								if (!_is_opaque(_get_block(x-1,y,z)))
									pov_tint_leftface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_A_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_A_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_A_STMY);
							break;
							case 0x1:
								if (!_is_opaque(_get_block(x,y+1,z)))
									pov_tint_upface   (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
								if (!_is_opaque(_get_block(x,y-1,z)))
									pov_tint_downface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
								if (!_is_opaque(_get_block(x,y,z+1)))
									pov_tint_frontface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
								if (!_is_opaque(_get_block(x,y,z-1)))
									pov_tint_backface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
								if (!_is_opaque(_get_block(x+1,y,z)))
									pov_tint_rightface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
								if (!_is_opaque(_get_block(x-1,y,z)))
									pov_tint_leftface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_B_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_B_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_B_STMY);
							break;
							case 0x2:
								if (!_is_opaque(_get_block(x,y+1,z)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)x/10.0,(float)(y+1)/10.0,(float)((-z)-1)/10.0);
								if (!_is_opaque(_get_block(x,y-1,z)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} rotate <90,0,0> translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
								if (!_is_opaque(_get_block(x,y,z+1)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
								if (!_is_opaque(_get_block(x,y,z-1)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} rotate <0,180,0> translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
								if (!_is_opaque(_get_block(x+1,y,z)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} rotate <0,-90,0> translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
								if (!_is_opaque(_get_block(x-1,y,z)))
									fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s][%.3f %s]}} rotate <0,90,0> translate <%.1f,%.1f,%.1f>}\n",0.1,MCPOV_LEAVES_A_GREY,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_ARID),MCPOV_LEAVES_A_ARID,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_COLD),MCPOV_LEAVES_A_COLD,
									_colortint(_biometint(MCMAP_OCEAN,0),_biometint(MCMAP_OCEAN,1),MCPOV_STMY),MCPOV_LEAVES_A_STMY, (float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
							break;
							case 0x3:
								if (!_is_opaque(_get_block(x,y+1,z)))
									pov_tint_upface   (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
								if (!_is_opaque(_get_block(x,y-1,z)))
									pov_tint_downface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
								if (!_is_opaque(_get_block(x,y,z+1)))
									pov_tint_frontface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
								if (!_is_opaque(_get_block(x,y,z-1)))
									pov_tint_backface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
								if (!_is_opaque(_get_block(x+1,y,z)))
									pov_tint_rightface(x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
								if (!_is_opaque(_get_block(x-1,y,z)))
									pov_tint_leftface (x,y,z,_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_LEAVES_C_ARID,
														_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_LEAVES_C_COLD,
														_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_LEAVES_C_STMY);
							break;
							}
					break;
					case MCMAP_TALL_GRASS:
						switch (_get_data(x,y,z))
							{
							default:
							case 0x00:
								fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
									MCPOV_DEAD_SHRUB,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
								fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>} translate <0,0,0>}\n",
									MCPOV_DEAD_SHRUB,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
							break;
							case 0x01:
								fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
									_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_TALL_GRASS_ARID,
									_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_TALL_GRASS_COLD,
									_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_TALL_GRASS_STMY,
									(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
								fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,45,0> translate <%f,%.1f,%f>} translate <0,0,0>}\n",
									_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_TALL_GRASS_ARID,
									_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_TALL_GRASS_COLD,
									_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_TALL_GRASS_STMY,
									(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
							break;
							case 0x02:
								fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
									_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_FERN_ARID,
									_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_FERN_COLD,
									_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_FERN_STMY,
									(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
								fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,45,0> translate <%f,%.1f,%f>} translate <0,0,0>}\n",
									_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_FERN_ARID,
									_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_FERN_COLD,
									_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_FERN_STMY,
									(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
							break;
							}
					break;
					case MCMAP_DEAD_BUSH:
						fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
							MCPOV_DEAD_SHRUB,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
						fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>}}\n",
							MCPOV_DEAD_SHRUB,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
					break;
					case MCMAP_DANDELION:
						fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
							MCPOV_DANDELION,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
						fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>}}\n",
							MCPOV_DANDELION,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
					break;
					case MCMAP_ROSE:
						fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
							MCPOV_ROSE,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
						fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>}}\n",
							MCPOV_ROSE,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
					break;
					case MCMAP_BROWN_MUSHROOM:
						fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
							MCPOV_BROWN_MUSHROOM,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
						fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>}}\n",
							MCPOV_BROWN_MUSHROOM,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
					break;
					case MCMAP_RED_MUSHROOM:
						fprintf(scene,"union {mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-45,0> translate <%f,%.1f,%f>}\n",
							MCPOV_RED_MUSHROOM,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)((-z)-1)/10.0+(0.1-sqrt(0.005))/2);
						fprintf(scene,"       mesh {triangle {<0,0,0><0,0.1,0><0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,45,0> translate <%f,%.1f,%f>}}\n",
							MCPOV_RED_MUSHROOM,(float)x/10.0+(0.1-sqrt(0.005))/2,(float)y/10.0,(float)(-z)/10.0-(0.1-sqrt(0.005))/2);
					break;
					case MCMAP_SNOW:
						s = ((float)(_get_data(x,y,z)&0x7)+1.0)*0.2;
						fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.5f,%.5f+(%.1f/16),%.5f>}\n",
							MCPOV_SNOW,(float)x/10.0,(float)y/10.0,s,(float)((-z)-1)/10.0);
						if (!_is_opaque(_get_block(x,y-1,z)))
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {%s} rotate <90,0,0> translate <%.5f,%.5f,%.5f>}\n",
								MCPOV_SNOW,(float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
						if (!_is_opaque(_get_block(x,y,z+1)))
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,%.1f/16,0>,<0.1,%.1f/16,0>} triangle {<0.1,0.2/16,0><0.1,0,0><0,0,0>} texture {%s} translate <%.5f,%.5f,%.5f>}\n",s,s,
								MCPOV_SNOW,(float)x/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
						if (!_is_opaque(_get_block(x,y,z-1)))
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,%.1f/16,0>,<0.1,%.1f/16,0>} triangle {<0.1,0.2/16,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,180,0> translate <%.5f,%.5f,%.5f>}\n",s,s,
								MCPOV_SNOW,(float)(x+1)/10.0,(float)y/10.0,(float)(-z)/10.0);
						if (!_is_opaque(_get_block(x+1,y,z)))
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,%.1f/16,0>,<0.1,%.1f/16,0>} triangle {<0.1,0.2/16,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,-90,0> translate <%.5f,%.5f,%.5f>}\n",s,s,
								MCPOV_SNOW,(float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0);
						if (!_is_opaque(_get_block(x-1,y,z)))
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,%.1f/16,0>,<0.1,%.1f/16,0>} triangle {<0.1,0.2/16,0><0.1,0,0><0,0,0>} texture {%s} rotate <0,90,0> translate <%.5f,%.5f,%.5f>}\n",s,s,
								MCPOV_SNOW,(float)x/10.0,(float)y/10.0,(float)(-z)/10.0);
					break;
					case MCMAP_CLAY_BLOCK:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_CLAY);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_CLAY);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_CLAY);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_CLAY);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_CLAY);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_CLAY);
					break;
					case MCMAP_VINES:
						if (_get_data(x,y,z)&0x4)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} translate <%.5f,%.5f,%.5f>}\n",
								_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_VINES_ARID,
								_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_VINES_COLD,
								_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_VINES_STMY,
								(float)x/10.0,(float)y/10.0,(float)(-z)/10.0-0.1/16.0);
						if (_get_data(x,y,z)&0x8)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,90,0> translate <%.5f,%.5f,%.5f>}\n",
								_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_VINES_ARID,
								_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_VINES_COLD,
								_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_VINES_STMY,
								(float)(x+1)/10.0-0.1/16.0,(float)y/10.0,(float)(-z)/10.0);
						if (_get_data(x,y,z)&0x1)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,180,0> translate <%.5f,%.5f,%.5f>}\n",
								_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_VINES_ARID,
								_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_VINES_COLD,
								_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_VINES_STMY,
								(float)(x+1)/10.0,(float)y/10.0,(float)((-z)-1)/10.0+0.1/16.0);
						if (_get_data(x,y,z)&0x2)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <0,-90,0> translate <%.5f,%.5f,%.5f>}\n",
								_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_VINES_ARID,
								_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_VINES_COLD,
								_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_VINES_STMY,
								(float)x/10.0+0.1/16.0,(float)y/10.0,(float)((-z)-1)/10.0);
						if (_can_vineover(_get_block(x,y+1,z)) || _get_data(x,y,z) == 0x0)
							fprintf(scene,"mesh {triangle {<0,0,0>,<0,0.1,0>,<0.1,0.1,0>} triangle {<0.1,0.1,0><0.1,0,0><0,0,0>} texture {average texture_map {[%.3f %s][%.3f %s][%.3f %s]}} rotate <-90,0,0> rotate <0,180,0> translate <%.5f,%.5f,%.5f>}\n",
								_colortint(pix_x,pix_y,MCPOV_ARID),MCPOV_VINES_ARID,
								_colortint(pix_x,pix_y,MCPOV_COLD),MCPOV_VINES_COLD,
								_colortint(pix_x,pix_y,MCPOV_STMY),MCPOV_VINES_STMY,
								(float)(x+1)/10.0,(float)(y+1)/10.0-0.1/16.0,(float)((-z)-1)/10.0);
					break;
					default:
						if (!_is_opaque(_get_block(x,y+1,z)))
							pov_upface   (x,y,z,MCPOV_NONE);
						if (!_is_opaque(_get_block(x,y-1,z)))
							pov_downface (x,y,z,MCPOV_NONE);
						if (!_is_opaque(_get_block(x,y,z+1)))
							pov_frontface(x,y,z,MCPOV_NONE);
						if (!_is_opaque(_get_block(x,y,z-1)))
							pov_backface (x,y,z,MCPOV_NONE);
						if (!_is_opaque(_get_block(x+1,y,z)))
							pov_rightface(x,y,z,MCPOV_NONE);
						if (!_is_opaque(_get_block(x-1,y,z)))
							pov_leftface (x,y,z,MCPOV_NONE);
					break;
					}
				}
			}
		}
	fclose(scene);
	
	//free memory
	for (rz=0; rz<=_get_region(max_z)-_get_region(min_z); rz++)
		{
		for (rx=0; rx<=_get_region(max_x)-_get_region(min_x); rx++)
			{
			for (cz=0; cz<32; cz++)
				{
				for (cx=0; cx<32; cx++)
					if (regions[rz][rx].chunks[cz][cx] != NULL)
						free(regions[rz][rx].chunks[cz][cx]);
				}
			}
		free(regions[rz]);
		}
	free(regions);
	
	return 0;
	}
