#ifndef __LIBNBT_HEADER
#define __LIBNBT_HEADER

#include <stdint.h>
#include <stdlib.h>

#define NBT_LIBNAME "libnbt"
#define NBT_MAXSTR 2048

//interpreting and encoding NBT file format < http://www.minecraftwiki.net/wiki/NBT_format >
//wrtten by Peter Markley, copyright 2013

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
	uint8_t islist; //is this a bare-minimum tag in a list?
	
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

//create and return pointer to nbt_tag based on contents of 'input' (compressed or uncompressed as specified by argument 3)
struct nbt_tag *nbt_decode(uint8_t *input, size_t input_sz, nbt_compression_type compress_type);

//free entire tag structure
void nbt_free_all(struct nbt_tag *);

//free one tag and its children from the linked structure and repair surrounding links
void nbt_free_one(struct nbt_tag *);

//print ASCII representation of NBT structure to the given FILE stream;
//print arrays with 'width' items per line;
//stop printing after 'maxlines' lines (-1 for unlimited)
void nbt_print_ascii(FILE *, struct nbt_tag *, int maxlines, int width);

#endif
