#include <zlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "nbt.h"

//operate zlib pipe from input to output, return size of output
size_t _nbt_decompress(uint8_t *input, uint8_t *output, size_t input_sz, nbt_compression_type compress_type)
	{
	size_t output_sz = 0;
	z_stream strm;
	int w_add; //number to add to 'windowBits' argument of 'inflateInit2()', based on compression type
	int ret;
	
	//we're responsible for allocating output
	if (output != NULL)
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
		fprintf(stderr,"%s ERROR: zlib returned an error: %s\n",NBT_LIBNAME,strm.msg);
		return 0;
		}
	//start out assuming a 2:1 compression ratio
	output_sz = input_sz*2;
	if ((output = (uint8_t *)calloc(output_sz,1)) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
		return 0;
		}
	strm.next_out = output;
	strm.avail_out = output_sz;
	
	//cycle through data, expanding output buffer as necessary
	while ((ret = inflate(&strm,Z_NO_FLUSH)) != Z_STREAM_END)
		{
		//check for errors
		if (ret != Z_OK)
			{
			fprintf(stderr,"%s ERROR: zlib returned an error: %s\n",NBT_LIBNAME,strm.msg);
			return 0;
			}
		//check the buffer size
		if (strm.avail_out == 0)
			{
			output_sz = output_sz*2; //let's go ahead and double it each time
			if ((output = realloc(output,output_sz)) == NULL)
				{
				fprintf(stderr,"%s ERROR: realloc() returned NULL\n",NBT_LIBNAME);
				return 0;
				}
			strm.next_out = &(output[strm.total_out+1]);
			strm.avail_out = output_sz-strm.total_out;
			}
		}
	
	//truncate unused memory
	output_sz -= strm.avail_out;
	if ((output = realloc(output,output_sz)) == NULL)
		{
		fprintf(stderr,"%s ERROR: realloc() returned NULL\n",NBT_LIBNAME);
		return 0;
		}
	return output_sz;
	}

//create and return pointer to nbt_tag based on contents of 'input' (compressed or uncompressed as specified by argument 3)
struct nbt_tag *nbt_decode(uint8_t *comp, size_t comp_sz, nbt_compression_type compress_type)
	{
	uint8_t *input = NULL; //input after decompression (which will be output from '_nbt_decompress()')
	size_t input_sz = 0;
	
	if (compress_type != NBT_COMPRESS_NONE)
		{
		if ((input_sz = _nbt_decompress(comp,input,comp_sz,compress_type)) == 0)
			return NULL;
		fprintf(stderr,"\tYAY! we might have %ld bytes of uncompressed data now at %p!\n",input_sz,input);
		//FIXME - do something with this instead of just freeing it
		free(input);
		}
	else
		{
		input = comp;
		input_sz = comp_sz;
		}
	
	return NULL;
	}

//free memory allocated in 'nbt_decode()' or 'nbt_new()'
void nbt_free(struct nbt_tag *t)
	{
	//FIXME
	return;
	}
