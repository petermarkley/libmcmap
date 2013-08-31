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
	return ( ((((int16_t)input[0])<<8)&0xFF00) +
	          (((int16_t)input[1])    &0x00FF) );
	}

//read a 32-bit, signed, big-endian integer from the given point in memory
int32_t _nbt_get_int(uint8_t *input)
	{
	return ( ((((int32_t)input[0])<<24)&0xFF000000) +
	         ((((int32_t)input[1])<<16)&0x00FF0000) +
	         ((((int32_t)input[2])<< 8)&0x0000FF00) +
	          (((int32_t)input[3])     &0x000000FF) );
	}

//take pointer to binary data 'input' of size 'limit';
//  allocate and populate nbt_tag pointed at by 't[0]' (owned by calling function
//  so they'll have access to it when we're done) whose parent should be 'parent' (NULL if root tag);
//  boolean 'islist' flag indicates whether tag should be an NBT_LIST;
//  return number of input bytes consumed, -1 on error
int _nbt_tag_read(uint8_t *input, size_t limit, struct nbt_tag **t, struct nbt_tag *parent, uint8_t islist)
	{
	unsigned int nextin = 0;
	int32_t num;
	int ret, i;
	struct nbt_tag **loop;
	
	//allocate tag
	if ((t[0] = (struct nbt_tag *)calloc(1,sizeof(struct nbt_tag))) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
		return -1;
		}
	//populate immediately known info
	t[0]->parent = parent;
	t[0]->islist = islist;
	
	//populate type and name
	if (islist) //if we're a member of a list, we have no name or independent tagid
		{
		if (parent == NULL) //the file root should not be a list
			{
			fprintf(stderr,"%s ERROR: list item with NULL parent\n",NBT_LIBNAME);
			return -1;
			}
		t[0]->type = parent->payload.p_list;
		t[0]->name = NULL;
		}
	else
		{
		//first byte of the tag is the ID
		t[0]->type = input[nextin++];
		//next two bytes are bytesize of name, followed by the name itself
		num = _nbt_get_short(&(input[nextin]));
		nextin += 2;
		if (num > 0)
			{
			//allocate space for name
			if ((t[0]->name = (char *)calloc(num+1,1)) == NULL)
				{
				fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
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
			fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
			return -1;
			}
		}
	
	//populate payload
	switch(t[0]->type)
		{
		case NBT_END:
			fprintf(stderr,"%s ERROR: unexpected compound tag terminator\n",NBT_LIBNAME);
			return -1;
			break;
		case NBT_BYTE:
			t[0]->payload.p_byte = input[nextin++];
			break;
		case NBT_SHORT:
			t[0]->payload.p_short = _nbt_get_short(&(input[nextin]));
			nextin += 2;
			break;
		case NBT_INT:
			t[0]->payload.p_int = _nbt_get_int(&(input[nextin]));
			nextin += 4;
			break;
		case NBT_LONG:
			t[0]->payload.p_long =
				( ((((int64_t)input[nextin  ])<<56)&0xFF00000000000000) +
				  ((((int64_t)input[nextin+1])<<48)&0x00FF000000000000) +
				  ((((int64_t)input[nextin+2])<<40)&0x0000FF0000000000) +
				  ((((int64_t)input[nextin+3])<<32)&0x000000FF00000000) +
				  ((((int64_t)input[nextin+4])<<24)&0x00000000FF000000) +
				  ((((int64_t)input[nextin+5])<<16)&0x0000000000FF0000) +
				  ((((int64_t)input[nextin+6])<< 8)&0x000000000000FF00) +
				   (((int64_t)input[nextin+7])     &0x00000000000000FF) );
			nextin += 8;
			break;
		case NBT_FLOAT:
			//FIXME - account for endianness
			memcpy(&(t[0]->payload.p_float),&(input[nextin]),4);
			nextin += 4;
			break;
		case NBT_DOUBLE:
			//FIXME - account for endianness
			memcpy(&(t[0]->payload.p_double),&(input[nextin]),8);
			nextin += 8;
			break;
		case NBT_BYTE_ARRAY:
			t[0]->payload.p_byte_array.size = _nbt_get_int(&(input[nextin]));
			nextin += 4;
			if (t[0]->payload.p_byte_array.size > 0)
				{
				if ((t[0]->payload.p_byte_array.data = (int8_t *)calloc(t[0]->payload.p_byte_array.size,1)) == NULL)
					{
					fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
					return -1;
					}
				memcpy(t[0]->payload.p_byte_array.data,&(input[nextin]),t[0]->payload.p_byte_array.size);
				nextin += t[0]->payload.p_byte_array.size;
				}
			break;
		case NBT_STRING:
			num = _nbt_get_short(&(input[nextin]));
			nextin += 2;
			if (num > 0)
				{
				if ((t[0]->payload.p_string = (char *)calloc(num,1)) == NULL)
					{
					fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
					return -1;
					}
				memcpy(t[0]->payload.p_string,&(input[nextin]),num);
				nextin += num;
				}
			break;
		case NBT_LIST:
			t[0]->payload.p_list = input[nextin++];
			num = _nbt_get_int(&(input[nextin]));
			nextin += 4;
			if (num > 0)
				{
				//allocate the first child at 't[0]->firstchild'
				loop = &(t[0]->firstchild);
				//read exactly 'num' number of children
				for (i=0; i<num; i++)
					{
					//read tag
					ret = _nbt_tag_read(&(input[nextin]),limit-nextin,loop,t[0],1);
					//recognize error condition
					if (ret == -1)
						return -1;
					//mark consumed input bytes
					nextin += ret;
					//check input range
					if (nextin >= limit)
						{
						fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
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
				ret = _nbt_tag_read(&(input[nextin]),limit-nextin,loop,t[0],0);
				//recognize error condition
				if (ret == -1)
					return -1;
				//mark consumed input bytes
				nextin += ret;
				//check input range
				if (nextin >= limit)
					{
					fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
					return -1;
					}
				//allocate next child as sibling of this one
				loop = &(loop[0]->next_sib);
				}
			
			//close the linked list
			for (loop = &(t[0]->firstchild); loop[0]->next_sib != NULL; loop = &(loop[0]->next_sib))
				loop[0]->next_sib->prev_sib = loop[0];
			
			//mark ending byte as consumed
			nextin++;
			
			break;
		case NBT_INT_ARRAY:
			t[0]->payload.p_int_array.size = _nbt_get_int(&(input[nextin]));
			nextin += 4;
			if (t[0]->payload.p_int_array.size > 0)
				{
				if ((t[0]->payload.p_int_array.data = (int32_t *)calloc(t[0]->payload.p_int_array.size,4)) == NULL)
					{
					fprintf(stderr,"%s ERROR: calloc() returned NULL\n",NBT_LIBNAME);
					return -1;
					}
				for (i=0; i < t[0]->payload.p_int_array.size; i++)
					{
					t[0]->payload.p_int_array.data[i] = _nbt_get_int(&(input[nextin]));
					nextin += 4;
					}
				}
			break;
		default:
			fprintf(stderr,"%s ERROR: unknown tag id\n",NBT_LIBNAME);
			return -1;
			break;
		}
	
	//check input range
	if (nextin > limit)
		{
		fprintf(stderr,"%s ERROR: premature end of stream\n",NBT_LIBNAME);
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
	
	//an NBT file is one big compound tag
	if (_nbt_tag_read(input,input_sz,&t,NULL,0) == -1)
		return NULL;
	
	if (input != comp) //don't free comp because we didn't allocate it
		free(input); //we only allocated if we ran it through zlib
	return t;
	}

//free entire tag structure
void nbt_free_all(struct nbt_tag *t)
	{
	if (t != NULL)
		{
		//children (parent is presumably being freed by the previous iteration)
		if (t->firstchild != NULL)
			nbt_free_all(t->firstchild);
		//subsequent siblings (previous siblings are being freed by the previous iteration)
		if (t->next_sib != NULL)
			nbt_free_all(t->next_sib);
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

//free one tag and its children from the linked structure and repair surrounding links
void nbt_free_one(struct nbt_tag *t)
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
		
		//ok we're untangled... now we can delete
		nbt_free_all(t);
		}
	return;
	}

//print ASCII representation of NBT structure to the given FILE stream;
//print arrays with 'width' items per line;
//stop printing after 'maxlines' lines (-1 for unlimited)
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
