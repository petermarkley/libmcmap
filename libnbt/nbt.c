#include <zlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "nbt.h"

//create and return pointer to nbt_tag based on contents of 'input' (compressed or uncompressed as specified by argument 3)
struct nbt_tag *nbt_decode(uint8_t *input, size_t input_sz, nbt_compression_type compress_type)
	{
	//FIXME
	return NULL;
	}

//free memory allocated in 'nbt_decode()' or 'nbt_new()'
void nbt_free(struct nbt_tag *t)
	{
	//FIXME
	return;
	}
