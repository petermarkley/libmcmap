//---------------------------------------------------------------------
//                         libnbt
//   Parsing and encoding the Minecraft Named Binary Tag format
//       < http://www.minecraftwiki.net/wiki/NBT_format >
// 
// Written by and Copyright 2013 Peter Markley <quartz@malexmedia.net>
// Distributed under the terms of the GNU General Public License.
// 
// Minecraft is the property of Mojang and a trademark of Notch
// Developement AB. Libnbt is developed by a third party under the
// terms provided at < http://minecraft.net/terms >.
// 
// Special thanks goes to Lukas Niederbremer <webmaster@flippeh.de>
// and Clark Gaebel <cg.wowus.cg@gmail.com> for a similar library
// distributed at < http://github.com/FliPPeh/cNBT >, which provided
// a working reference for libnbt.
// 
// This file is part of libnbt.
// 
// Libnbt is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Libnbt is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with libnbt.  If not, see < http://www.gnu.org/licenses/ >.
// 
//---------------------------------------------------------------------

#ifndef __LIBNBT_HEADER
#define __LIBNBT_HEADER

#include <stdint.h>
#include <stdlib.h>

#define NBT_LIBNAME "libnbt"
#define NBT_MAXSTR 2048

char nbt_error[NBT_MAXSTR]; //in error conditions, this will be populated with a detailed human-readable message

typedef enum //tag type
	{
	NBT_END        =  0, //this should not appear in the memory structure
	NBT_BYTE       =  1, // 8-bit signed value
	NBT_SHORT      =  2, //16-bit, signed, big-endian integer
	NBT_INT        =  3, //32-bit, signed, big-endian integer
	NBT_LONG       =  4, //64-bit, signed, big-endian integer
	NBT_FLOAT      =  5, //32-bit, signed, big-endian IEEE 754-2008 'binary32' < http://en.wikipedia.org/wiki/Single_precision_floating-point_format >
	NBT_DOUBLE     =  6, //64-bit, signed, big-endian IEEE 754-2008 'binary64' < http://en.wikipedia.org/wiki/Double_precision_floating-point_format >
	NBT_BYTE_ARRAY =  7, //one NBT_INT payload followed by that many payloads of type NBT_BYTE
	NBT_STRING     =  8, //one NBT_SHORT payload followed by that many bytes of UTF-8 text
	NBT_LIST       =  9, //one NBT_BYTE payload signifying a tag id, then one NBT_INT payload, then that many payloads of that type
	NBT_COMPOUND   = 10, //tags, each with an id and name, terminated by an NBT_END
	NBT_INT_ARRAY  = 11  //one NBT_INT payload followed by that many NBT_INTs
	} nbt_tagid;

typedef enum //compression type
	{
	NBT_COMPRESS_NONE = 0, //uncompressed
	NBT_COMPRESS_GZIP = 1, //RFC 1952
	NBT_COMPRESS_ZLIB = 2, //RFC 1950
	NBT_COMPRESS_UNKNOWN = -1
	} nbt_compression_type;

//we're not gonna mess with any bit-for-bit struct-to-memory mapping nonsense here like we do in libmcmap
struct nbt_tag
	{
	nbt_tagid type; //access only the corresponding payload
	char *name; //technically can be UTF-8, but may be safely treated as ASCII
	
	//payloads
	union
		{
		int8_t    p_byte;
		int16_t   p_short;
		int32_t   p_int;
		int64_t   p_long;
		float     p_float;
		double    p_double;
		struct nbt_tag_byte_array
			{
			int32_t size;
			int8_t *data;
			}     p_byte_array;
		char     *p_string;
		nbt_tagid p_list; //lists are treated as compound tags for simplicity
		struct nbt_tag_int_array
			{
			int32_t size;
			int32_t *data;
			}     p_int_array;
		} payload;
	
	struct nbt_tag *parent; //only for the root tag will this be NULL
	struct nbt_tag *prev_sib;
	struct nbt_tag *next_sib;
	struct nbt_tag *firstchild; //NULL for all but compound tags or lists; will allocate array of size 'children_num'
	//unsigned int children_num; //0 for all but compound tags or lists
	};

//create and return pointer to nbt_tag based on contents of 'input' with anticipated compression type 'compress_type' (may be NBT_COMPRESS_UNKNOWN);
//return NULL on failure
struct nbt_tag *nbt_decode(uint8_t *input, size_t input_sz, nbt_compression_type compress_type);

//allocate 'output[0]' and save contents of 't' to it with compression type 'compress_type' (may NOT be NBT_COMPRESS_UNKNOWN);
//return size of 'output[0]' buffer or -1 on failure
int nbt_encode(struct nbt_tag *t, uint8_t **output, nbt_compression_type compress_type);

//load an NBT structure from a file on the disk, return NULL on failure
struct nbt_tag *nbt_file_read(const char *);

//free entire tag structure
void nbt_free(struct nbt_tag *);

//separate one tag and its children from the linked structure, return it as its own root tag, and repair surrounding links
//return value can immediately be passed to 'nbt_free()' if desired: 'nbt_free(nbt_separate(tag));'
struct nbt_tag *nbt_separate(struct nbt_tag *);

//locate & return a particular child of a compound or list tag by its type and name; return NULL if not found
struct nbt_tag *nbt_find_child(struct nbt_tag *, nbt_tagid, const char *);

//print ASCII representation of NBT structure to the given FILE stream;
//print arrays with 'width' items per line;
//stop printing array data after 'maxlines' lines (-1 for unlimited)
void nbt_print_ascii(FILE *, struct nbt_tag *, int maxlines, int width);

#endif
