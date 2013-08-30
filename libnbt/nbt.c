#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "nbt.h"

//operate zlib pipe from input to output, return size of output
size_t _nbt_decompress(uint8_t *input, uint8_t **output, size_t input_sz, nbt_compression_type compress_type)
	{
	size_t output_sz = 0;
	z_stream strm;
	int w_add; //number to add to 'windowBits' argument of 'inflateInit2()', based on compression type
	int ret;
	
	//we're responsible for allocating output
	if (output[0] != NULL)
		{
		fprintf(stderr,"%s ERROR: _nbt_decompress() was passed a non-null 'output' pointer for allocation\n",NBT_LIBNAME);
		return 0;
		}
	switch (compress_type)
		{
		case NBT_COMPRESS_NONE:    return 0; break;
		case NBT_COMPRESS_GZIP:    w_add=16; break;
		case NBT_COMPRESS_ZLIB:    w_add= 0; break;
		case NBT_COMPRESS_UNKNOWN: w_add=32; break;
		}
	
	//initialize!
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = input;
	strm.avail_in = input_sz;
	if (inflateInit2(&strm,15+w_add) != Z_OK)
		{
		fprintf(stderr,"%s ERROR: zlib inflateInit2() failed: %s\n",NBT_LIBNAME,strm.msg);
		return 0;
		}
	//start out assuming a 2:1 compression ratio
	output_sz = input_sz*2;
	if ((output[0] = (uint8_t *)calloc(output_sz,1)) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
		return 0;
		}
	strm.next_out = output[0];
	strm.avail_out = output_sz;
	
	//cycle through data, expanding output buffer as necessary
	while ((ret = inflate(&strm,Z_NO_FLUSH)) != Z_STREAM_END)
		{
		//check for errors
		if (ret != Z_OK)
			{
			switch (ret)
				{
				case Z_DATA_ERROR: fprintf(stderr,"\tZ_DATA_ERROR\n"); break;
				case Z_STREAM_ERROR: fprintf(stderr,"\tZ_STREAM_ERROR\n"); break;
				case Z_MEM_ERROR: fprintf(stderr,"\tZ_MEM_ERROR\n"); break;
				case Z_BUF_ERROR: fprintf(stderr,"\tZ_BUF_ERROR\n"); break;
				}
			fprintf(stderr,"%s ERROR: zlib inflate() failed: %s\n",NBT_LIBNAME,strm.msg);
			return 0;
			}
		//check the buffer size
		if (strm.avail_out == 0)
			{
			output_sz = output_sz*2; //let's go ahead and double it each time
			if ((output[0] = realloc(output[0],output_sz)) == NULL)
				{
				fprintf(stderr,"%s ERROR: realloc() returned NULL\n",NBT_LIBNAME);
				return 0;
				}
			strm.next_out = &(output[0][strm.total_out+1]);
			strm.avail_out = output_sz-strm.total_out;
			}
		}
	
	//truncate unused memory
	output_sz -= strm.avail_out;
	if ((output[0] = realloc(output[0],output_sz)) == NULL)
		{
		fprintf(stderr,"%s ERROR: realloc() returned NULL\n",NBT_LIBNAME);
		return 0;
		}
	return output_sz;
	}

//read a 16-bit, signed, big-endian integer from the given point in memory
int16_t _nbt_get_short(uint8_t *input)
	{
	return ( ((int16_t)(input[0])<<8) + (int16_t)(input[1]) );
	}

//read a 32-bit, signed, big-endian integer from the given point in memory
int32_t _nbt_get_int(uint8_t *input)
	{
	return ( ((int32_t)(input[0])<<24) + ((int32_t)(input[1])<<16) + ((int32_t)(input[2])<<8) + (int32_t)(input[3]) );
	}

//create tag from binary input and return pointer to it
struct nbt_tag *_nbt_tag_read(uint8_t *input, size_t limit, struct nbt_tag *parent, uint8_t islist)
	{
	struct nbt_tag *t;
	unsigned int nextin = 0;
	uint32_t num;
	if ((t = (struct nbt_tag *)calloc(1,sizeof(struct nbt_tag))) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
		return NULL;
		}
	t->parent = parent;
	t->islist = islist;
	
	if (islist) //if we're a member of a list, we have no name or independent tagid
		{
		if (parent == NULL) //the file root should not be a list
			{
			fprintf(stderr,"%s ERROR: list item with NULL parent\n",NBT_LIBNAME);
			return NULL;
			}
		t->type = parent->payload.p_list;
		t->name = NULL;
		}
	else
		{
		//first byte of the tag is the ID
		t->type = input[nextin++];
		//next two bytes are bytesize of name, followed by the name itself
		num = _nbt_get_short(&(input[nextin]));
		nextin += 2;
		//allocate space for name
		if ((t->name = (char *)calloc(num,sizeof(char))) == NULL)
			{
			fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
			return NULL;
			}
		//store name
		memcpy(t->name,&(input[nextin]),num);
		nextin += num;
		//check range of input
		if (nextin >= limit)
			{
			fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
			return NULL;
			}
		}
	
	//populate payload
	switch(t->type)
		{
		case NBT_END:
			fprintf(stderr,"%s ERROR: unexpected compound tag terminator\n",NBT_LIBNAME);
			return NULL;
			break;
		case NBT_BYTE:
			t->payload.p_byte = input[nextin++];
			break;
		case NBT_SHORT:
			t->payload.p_short = _nbt_get_short(&(input[nextin]));
			nextin += 2;
			break;
		case NBT_INT:
			t->payload.p_int = _nbt_get_int(&(input[nextin]));
			nextin += 4;
			break;
		case NBT_LONG:
			
			break;
		case NBT_FLOAT:
			
			break;
		case NBT_DOUBLE:
			
			break;
		case NBT_BYTE_ARRAY:
			
			break;
		case NBT_STRING:
			
			break;
		case NBT_LIST:
			
			break;
		case NBT_COMPOUND:
			t->children = _nbt_tag_read(input,limit-nextin,t,0);
			//FIXME need for loop here until we encounter NBT_END
			break;
		case NBT_INT_ARRAY:
			
			break;
		default:
			fprintf(stderr,"%s ERROR: unknown tag id\n",NBT_LIBNAME);
			return NULL;
			break;
		}
	
	//check range of input
	if (nextin > limit)
		{
		fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
		return NULL;
		}
	return t;
	}

//create and return pointer to nbt_tag based on contents of 'input' (compressed or uncompressed as specified by argument 3)
struct nbt_tag *nbt_decode(uint8_t *comp, size_t comp_sz, nbt_compression_type compress_type)
	{
	uint8_t *input = NULL; //input after decompression (which will be output from '_nbt_decompress()')
	size_t input_sz = 0;
	struct nbt_tag *t;
	
	if (compress_type != NBT_COMPRESS_NONE)
		{
		if ((input_sz = _nbt_decompress(comp,&input,comp_sz,compress_type)) == 0)
			return NULL;
		}
	else
		{
		input = comp;
		input_sz = comp_sz;
		}
	fprintf(stderr,"\tYAY! we might have %d bytes of uncompressed data now at %p!\n",(int)input_sz,input);
	//an NBT file is one big compound tag
	t = _nbt_tag_read(input,input_sz,NULL,0);
	
	if (input != comp) //don't free comp because we didn't allocate it
		free(input); //we only allocated if we ran it through zlib
	return t;
	}

//free memory allocated in 'nbt_decode()' or 'nbt_new()'
void nbt_free(struct nbt_tag *t)
	{
	//FIXME
	return;
	}
