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

#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "nbt.h"
#include "cswap.h"

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
		snprintf(nbt_error,NBT_MAXSTR,"_nbt_decompress() was passed a non-null 'output' pointer for allocation");
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
		snprintf(nbt_error,NBT_MAXSTR,"zlib inflateInit2(): %s",strm.msg);
		return 0;
		}
	//start out assuming a 2:1 compression ratio
	output_sz = input_sz*2;
	if ((output[0] = (uint8_t *)calloc(output_sz,1)) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
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
			snprintf(nbt_error,NBT_MAXSTR,"zlib inflate(): %s",strm.msg);
			return 0;
			}
		//check the buffer size
		if (strm.avail_out == 0)
			{
			output_sz = output_sz*2; //let's go ahead and double it each time
			if ((output[0] = realloc(output[0],output_sz)) == NULL)
				{
				snprintf(nbt_error,NBT_MAXSTR,"realloc() returned NULL");
				return 0;
				}
			strm.next_out = &(output[0][strm.total_out]);
			strm.avail_out = output_sz-strm.total_out;
			}
		}
	
	//truncate unused memory
	output_sz -= strm.avail_out;
	if ((output[0] = realloc(output[0],output_sz)) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"realloc() returned NULL");
		return 0;
		}
	return output_sz;
	}

//take pointer to binary data 'input' of size 'limit';
//  allocate and populate nbt_tag pointed at by 't[0]' (owned by calling function
//  so they'll have access to it when we're done) whose parent should be 'parent' (NULL if root tag);
//  return number of input bytes consumed, -1 on error
int _nbt_tag_read(uint8_t *input, size_t limit, struct nbt_tag **t, struct nbt_tag *parent)
	{
	unsigned int nextin = 0;
	int32_t num;
	int ret, i;
	struct nbt_tag **loop;
	uint32_t tmp1;
	uint64_t tmp2;
	float *p1;
	double *p2;
	
	//allocate tag
	if ((t[0] = (struct nbt_tag *)calloc(1,sizeof(struct nbt_tag))) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
		return -1;
		}
	t[0]->parent = parent;
	
	//populate type and name
	if (t[0]->parent != NULL && t[0]->parent->type == NBT_LIST)
		{
		t[0]->type = parent->payload.p_list; //list items have no independent tag ID
		t[0]->name = NULL; //list items have no name
		}
	else
		{
		//first byte of the tag is the ID
		t[0]->type = input[nextin++];
		//next two bytes are bytesize of name, followed by the name itself
		num = cswapr_16(&(input[nextin]));
		nextin += 2;
		if (num > 0)
			{
			//allocate space for name
			if ((t[0]->name = (char *)calloc(num+1,1)) == NULL)
				{
				snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
				return -1;
				}
			//store name
			memcpy(t[0]->name,&(input[nextin]),num);
			t[0]->name[num] = '\0';
			nextin += num;
			}
		//check range of input
		if (nextin >= limit)
			{
			snprintf(nbt_error,NBT_MAXSTR,"premature end of stream");
			return -1;
			}
		}
	
	//populate payload
	switch(t[0]->type)
		{
		case NBT_END:
			snprintf(nbt_error,NBT_MAXSTR,"unexpected compound tag terminator");
			return -1;
			break;
		case NBT_BYTE:
			t[0]->payload.p_byte = input[nextin++];
			break;
		case NBT_SHORT:
			t[0]->payload.p_short = cswapr_16(&(input[nextin]));
			nextin += 2;
			break;
		case NBT_INT:
			t[0]->payload.p_int = cswapr_32(&(input[nextin]));
			nextin += 4;
			break;
		case NBT_LONG:
			t[0]->payload.p_long = cswapr_64(&(input[nextin]));
			nextin += 8;
			break;
		case NBT_FLOAT:
			tmp1 = cswapr_32(&(input[nextin]));
			p1 = (float *)&tmp1;
			nextin += 4;
			t[0]->payload.p_float = *p1;
			break;
		case NBT_DOUBLE:
			tmp2 = cswapr_64(&(input[nextin]));
			p2 = (double *)&tmp2;
			nextin += 8;
			t[0]->payload.p_double = *p2;
			break;
		case NBT_BYTE_ARRAY:
			t[0]->payload.p_byte_array.size = cswapr_32(&(input[nextin]));
			nextin += 4;
			if (t[0]->payload.p_byte_array.size > 0)
				{
				if ((t[0]->payload.p_byte_array.data = (int8_t *)calloc(t[0]->payload.p_byte_array.size,1)) == NULL)
					{
					snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
					return -1;
					}
				memcpy(t[0]->payload.p_byte_array.data,&(input[nextin]),t[0]->payload.p_byte_array.size);
				nextin += t[0]->payload.p_byte_array.size;
				}
			break;
		case NBT_STRING:
			num = cswapr_16(&(input[nextin]));
			nextin += 2;
			if (num > 0)
				{
				if ((t[0]->payload.p_string = (char *)calloc(num,1)) == NULL)
					{
					snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
					return -1;
					}
				memcpy(t[0]->payload.p_string,&(input[nextin]),num);
				nextin += num;
				}
			break;
		case NBT_LIST:
			t[0]->payload.p_list = input[nextin++];
			num = cswapr_32(&(input[nextin]));
			nextin += 4;
			if (num > 0)
				{
				//allocate the first child at 't[0]->firstchild'
				loop = &(t[0]->firstchild);
				//read exactly 'num' number of children
				for (i=0; i<num; i++)
					{
					//read tag
					ret = _nbt_tag_read(&(input[nextin]),limit-nextin,loop,t[0]);
					//recognize error condition
					if (ret == -1)
						return -1;
					//mark consumed input bytes
					nextin += ret;
					//check input range
					if (nextin >= limit)
						{
						snprintf(nbt_error,NBT_MAXSTR,"premature end of stream");
						return -1;
						}
					//allocate next child as sibling of this one
					loop = &(loop[0]->next_sib);
					}
				//close the linked list
				for (loop = &(t[0]->firstchild); loop[0]->next_sib != NULL; loop = &(loop[0]->next_sib))
					loop[0]->next_sib->prev_sib = loop[0];
				}
			break;
		case NBT_COMPOUND:
			//allocate the first child at 't[0]->firstchild'
			loop = &(t[0]->firstchild);
			
			//keep reading children until we encounter an 'NBT_END' tag
			while (input[nextin] != NBT_END)
				{
				//read tag
				ret = _nbt_tag_read(&(input[nextin]),limit-nextin,loop,t[0]);
				//recognize error condition
				if (ret == -1)
					return -1;
				//mark consumed input bytes
				nextin += ret;
				//check input range
				if (nextin >= limit)
					{
					snprintf(nbt_error,NBT_MAXSTR,"premature end of stream");
					return -1;
					}
				//allocate next child as sibling of this one
				loop = &(loop[0]->next_sib);
				}
			
			//close the linked list
			if (t[0]->firstchild != NULL)
				for (loop = &(t[0]->firstchild); loop[0]->next_sib != NULL; loop = &(loop[0]->next_sib))
					loop[0]->next_sib->prev_sib = loop[0];
			
			//mark ending byte as consumed
			nextin++;
			
			break;
		case NBT_INT_ARRAY:
			t[0]->payload.p_int_array.size = cswapr_32(&(input[nextin]));
			nextin += 4;
			if (t[0]->payload.p_int_array.size > 0)
				{
				if ((t[0]->payload.p_int_array.data = (int32_t *)calloc(t[0]->payload.p_int_array.size,4)) == NULL)
					{
					snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
					return -1;
					}
				for (i=0; i < t[0]->payload.p_int_array.size; i++)
					{
					t[0]->payload.p_int_array.data[i] = cswapr_32(&(input[nextin]));
					nextin += 4;
					}
				}
			break;
		default:
			snprintf(nbt_error,NBT_MAXSTR,"unknown tag id");
			return -1;
			break;
		}
	
	//check input range
	if (nextin > limit)
		{
		snprintf(nbt_error,NBT_MAXSTR,"premature end of stream");
		return -1;
		}
	return nextin;
	}

//create and return pointer to nbt_tag based on contents of 'input' (compressed or uncompressed as specified by argument 3)
struct nbt_tag *nbt_decode(uint8_t *comp, size_t comp_sz, nbt_compression_type compress_type)
	{
	uint8_t *input = NULL; //input after decompression (which will be output from '_nbt_decompress()')
	size_t input_sz = 0;
	struct nbt_tag *t;
	//FILE *f;
	
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
	
	//write file for diagnostics
	/*if ((f = fopen("/tmp/temp.nbt","w")) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"fopen() returned NULL");
		return NULL;
		}
	if (fwrite(input,1,input_sz,f) != input_sz)
		{
		snprintf(nbt_error,NBT_MAXSTR,"fwrite() returned short item count");
		return NULL;
		}
	fclose(f);*/
	
	//an NBT file is one big compound tag
	if (_nbt_tag_read(input,input_sz,&t,NULL) == -1)
		return NULL;
	
	if (input != comp) //don't free comp because we didn't allocate it
		free(input); //we only allocated if we ran it through zlib
	return t;
	}

//load an NBT structure from a file on the disk
struct nbt_tag *nbt_file_read(const char *fn)
	{
	struct nbt_tag *t;
	FILE *f;
	struct stat fs;
	uint8_t *b;
	int i;
	
	//open file...
	if ((f = fopen(fn,"r")) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"fopen() on \'%s\': %s",fn,strerror(errno));
		return NULL;
		}
	//determine filesize...
	if (fstat(fileno(f),&fs) != 0)
		{
		snprintf(nbt_error,NBT_MAXSTR,"fstat() on \'%s\': %s",fn,strerror(errno));
		return NULL;
		}
	//allocate buffer...
	if ((b = (uint8_t *)calloc(fs.st_size,1)) == NULL)
		{
		snprintf(nbt_error,NBT_MAXSTR,"calloc() returned NULL");
		return NULL;
		}
	//copy file to buffer...
	if ((i = fread(b,1,fs.st_size,f)) != fs.st_size)
		{
		snprintf(nbt_error,NBT_MAXSTR,"fread() encountered %s on the last %d requested bytes of \'%s\'",(ferror(f)?"an error":"EOF"),(unsigned int)fs.st_size-i,fn);
		return NULL;
		}
	//don't need this anymore...
	fclose(f);
	//parse buffer...
	if ((t = nbt_decode(b,fs.st_size,NBT_COMPRESS_UNKNOWN)) == NULL)
		{
		if ((t = nbt_decode(b,fs.st_size,NBT_COMPRESS_NONE)) == NULL)
			{
			snprintf(nbt_error,NBT_MAXSTR,"couldn't determine how to read \'%s\'; file corrupted?",fn);
			return NULL;
			}
		}
	//don't need this anymore...
	free(b);
	
	return t;
	}

//free entire tag structure
void nbt_free(struct nbt_tag *t)
	{
	if (t != NULL)
		{
		//children (parent is presumably being freed by the previous iteration)
		if (t->firstchild != NULL)
			nbt_free(t->firstchild);
		//subsequent siblings (previous siblings are being freed by the previous iteration)
		if (t->next_sib != NULL)
			nbt_free(t->next_sib);
		//name
		if (t->name != NULL)
			free(t->name);
		//payload
		switch(t->type) //don't test for NULL on union members that weren't used; the memory space was still used since it's a union, right? test would have unpredictable results
			{
			case NBT_BYTE_ARRAY:
				if (t->payload.p_byte_array.data != NULL)
					free(t->payload.p_byte_array.data);
				break;
			case NBT_STRING:
				if (t->payload.p_string != NULL)
					free(t->payload.p_string);
				break;
			case NBT_INT_ARRAY:
				if (t->payload.p_int_array.data != NULL)
					free(t->payload.p_int_array.data);
				break;
			default: break;
			}
		//whole tag
		free(t);
		}
	return;
	}

//separate one tag and its children from the linked structure, return it as its own root tag, and repair surrounding links
//return value can immediately be passed to 'nbt_free()' if desired: 'nbt_free(nbt_separate(tag));'
struct nbt_tag *nbt_separate(struct nbt_tag *t)
	{
	if (t != NULL)
		{
		if (t->prev_sib == NULL) //is this the first child?
			{
			if (t->next_sib != NULL) //are there other children?
				{
				t->next_sib->prev_sib = NULL;
				if (t->parent != NULL)
					t->parent->firstchild = t->next_sib;
				}
			else
				if (t->parent != NULL)
					t->parent->firstchild = NULL;
			}
		else //not the first child
			{
			if (t->next_sib != NULL) //are there later children?
				{
				t->prev_sib->next_sib = t->next_sib;
				t->next_sib->prev_sib = t->prev_sib;
				}
			else
				t->prev_sib->next_sib = NULL;
			}
		t->prev_sib = NULL;
		t->next_sib = NULL;
		t->parent = NULL;
		}
	return t;
	}

//locate & return a particular child of a compound or list tag by its type and name; return NULL if not found
struct nbt_tag *nbt_find_child(struct nbt_tag *t, nbt_tagid type, const char *name)
	{
	struct nbt_tag *loop;
	if (t == NULL || t->firstchild == NULL)
		return NULL;
	for (loop = t->firstchild; loop != NULL; loop = loop->next_sib)
		{
		if (loop->type == type)
			{
			if (name == NULL)
				{
				if (loop->name == NULL)
					return loop;
				}
			else
				{
				if (loop->name != NULL && strcmp(loop->name,name) == 0)
					return loop;
				}
			}
		}
	return NULL;
	}

//print ASCII representation of NBT structure to the given FILE stream;
//print arrays with 'width' items per line;
//stop printing array data after 'maxlines' lines (-1 for unlimited)
void nbt_print_ascii(FILE *f, struct nbt_tag *t, int maxlines, int width)
	{
	char c[NBT_MAXSTR];
	char b[NBT_MAXSTR];
	struct nbt_tag *probe;
	int i,j;
	
	if (t == NULL)
		return;
	c[0] = '\0';
	b[0] = '\0';
	
	//formatting prefix
	for (probe = t; probe->parent != NULL; probe = probe->parent)
		{
		if (probe != t)
			{
			if (probe->next_sib != NULL)
				snprintf(b,NBT_MAXSTR,"|    %s",c);
			else
				snprintf(b,NBT_MAXSTR,"     %s",c);
			}
		strncpy(c,b,NBT_MAXSTR);
		}
	if (t->parent != NULL)
		{
		if (t->next_sib != NULL)
			snprintf(b,NBT_MAXSTR,"+-- ");
		else
			snprintf(b,NBT_MAXSTR,"L-- ");
		fprintf(f," %s%s",c,b);
		}
	if (t->next_sib != NULL)
		snprintf(b,NBT_MAXSTR,"|    ");
	else
		snprintf(b,NBT_MAXSTR,"     ");
	
	//tag type
	switch (t->type)
		{
		case NBT_BYTE:       fprintf(f,"Byte");       break;
		case NBT_SHORT:      fprintf(f,"Short");      break;
		case NBT_INT:        fprintf(f,"Int");        break;
		case NBT_LONG:       fprintf(f,"Long");       break;
		case NBT_FLOAT:      fprintf(f,"Float");      break;
		case NBT_DOUBLE:     fprintf(f,"Double");     break;
		case NBT_BYTE_ARRAY: fprintf(f,"Byte Array"); break;
		case NBT_STRING:     fprintf(f,"String");     break;
		case NBT_LIST:       fprintf(f,"List");       break;
		case NBT_COMPOUND:   fprintf(f,"Compound");   break;
		case NBT_INT_ARRAY:  fprintf(f,"Int Array");  break;
		default: break;
		}
	
	//tag name
	if (t->name != NULL)
		fprintf(f," \"%s\":  ",t->name);
	else
		fprintf(f,":  ");
	
	//payload
	switch (t->type)
		{
		case NBT_BYTE:   fprintf(f,"%02x",t->payload.p_byte);  break;
		case NBT_SHORT:  fprintf(f,"%d",t->payload.p_short);   break;
		case NBT_INT:    fprintf(f,"%d",t->payload.p_int);     break;
		case NBT_LONG:   fprintf(f,"%ld",(long)t->payload.p_long);   break;
		case NBT_FLOAT:  fprintf(f,"%f",t->payload.p_float);   break;
		case NBT_DOUBLE: fprintf(f,"%lf",t->payload.p_double); break;
		case NBT_BYTE_ARRAY:
			fprintf(f,"(%d items)",t->payload.p_byte_array.size);
			j=0; //'i' is array index, 'j' is a line counter
			if (maxlines != 0)
				{
				for (i=0; i < t->payload.p_byte_array.size;)
					{
					fprintf(f,"\n %s%s    %02x",c,b,(uint8_t)t->payload.p_byte_array.data[i++]);
					while (i%width > 0 && i < t->payload.p_byte_array.size)
						fprintf(f," %02x",(uint8_t)t->payload.p_byte_array.data[i++]);
					j++;
					if (maxlines > 0)
						{
						if (j >= maxlines)
							{
							i = t->payload.p_byte_array.size; //end loop
							fprintf(f,"\n %s%s    . . .",c,b); //drop ellipsis
							}
						}
					}
				}
			else
				fprintf(f," . . .");
			break;
		case NBT_STRING:    fprintf(f,"%s",((t->payload.p_string != NULL)?t->payload.p_string:"(null)")); break;
		case NBT_LIST: break;
		case NBT_COMPOUND: break;
		case NBT_INT_ARRAY:
			fprintf(f,"(%d items)",t->payload.p_int_array.size);
			j=0; //'i' is array index, 'j' is a line counter
			if (maxlines != 0)
				{
				for (i=0; i < t->payload.p_int_array.size;)
					{
					fprintf(f,"\n %s%s    %d",c,b,t->payload.p_int_array.data[i++]);
					while (i%width > 0 && i < t->payload.p_int_array.size)
						fprintf(f," %d",t->payload.p_int_array.data[i++]);
					j++;
					if (maxlines > 0)
						{
						if (j >= maxlines)
							{
							i = t->payload.p_int_array.size; //end loop
							fprintf(f,"\n %s%s    . . .",c,b); //drop ellipsis
							}
						}
					}
				}
			else
				fprintf(f," . . .");
			break;
		default: break;
		}
	
	fprintf(f,"\n");
	
	//children
	if (t->firstchild != NULL)
		nbt_print_ascii(f,t->firstchild,maxlines,width);
	//subsequent siblings
	if (t->next_sib != NULL)
		nbt_print_ascii(f,t->next_sib,maxlines,width);
	
	return;
	}
