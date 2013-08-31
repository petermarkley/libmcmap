#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "mcmap.h"
#include "libnbt/nbt.h"
#include "cswap.h"

//searches the given path to a minecraft map folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; if error returns NULL
struct mcmap_region *mcmap_region_read(int ix, int iz, char *path)
	{
	FILE *r_file;
	char r_name[MCMAP_MAXSTR];
	struct stat r_stat;
	uint8_t *buff;
	struct mcmap_region *r;
	uint32_t l;
	unsigned int x,z,i,err;
	
	//resolve filename from map directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		snprintf(r_name, MCMAP_MAXSTR, "%sregion/r.%d.%d.mca", path, ix, iz);
	else
		snprintf(r_name, MCMAP_MAXSTR, "%s/region/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((r_file = fopen(r_name,"r")) == NULL)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fopen() on \'%s\': %s",r_name,strerror(errno));
		return NULL;
		}
	//determine filesize...
	if (fstat(fileno(r_file),&r_stat) != 0)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"fstat() on \'%s\': %s",r_name,strerror(errno));
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
		snprintf(mcmap_error,MCMAP_MAXSTR,"fread() encountered %s on the last %d requested bytes of \'%s\'",(ferror(r_file)?"an error":"EOF"),(unsigned int)r_stat.st_size-i,r_name);
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
	err = 0;
	for (z=0;z<32 && !err;z++)
		{
		for (x=0;x<32 && !err;x++)
			{
			if (r->header->locations[z][x].sector_count > 0)
				{
				//extract big-endian 32-bit integer from r->header->dates[z][x]
				r->dates[z][x] = cswap_32(&(r->header->dates[z][x]));
				//extract big-endian 24-bit integer from r->header->location[z][x].offset
				l = cswap_24(&(r->header->locations[z][x].offset));
				
				//chunk listing should not point anywhere in the file header
				if (l < 2)
					err = 1;
				//chunk listing should not point past the end of the file
				i = l*4096;
				if (i+r->header->locations[z][x].sector_count*4096 >= r_stat.st_size)
					err = 1;
				
				if (!err)
					{
					//connect 5-byte chunk header
					r->chunks[z][x].header = (struct mcmap_region_chunk_header *)&(buff[i]);
					//extract big-endian 32-bit integer from r->chunks[z][x].header->length (same location as buff[i])
					r->chunks[z][x].size = cswap_32(&(buff[i]));
					//'r->chunks[z][x].data' will now point to a block of 'r->chunks[z][x].size' bytes
					r->chunks[z][x].data = &(buff[i+5]);
					
					//listed chunk size should not be larger than the rest of the file
					if (i+5+r->chunks[z][x].size >= r_stat.st_size)
						err = 1;
					//in fact neither should it be larger than the sector count in the file header
					if (r->chunks[z][x].size+5 > r->header->locations[z][x].sector_count*4096)
						err = 1;
					}
				}
			else
				{
				r->chunks[z][x].header = NULL;
				r->chunks[z][x].size = 0;
				r->chunks[z][x].data = NULL;
				}
			}
		}
	if (err)
		{
		snprintf(mcmap_error,MCMAP_MAXSTR,"file \'%s\' may be correupted",r_name);
		return NULL;
		}
	
	return r;
	}

//free all memory allocated in 'mcmap_region_read()' or 'mcmap_region_new()'
void mcmap_region_free(struct mcmap_region *r)
	{
	free(r->header); //free the memory buffer of the file contents
	free(r); //free the navigation nodes that were connected throughout the buffer
	return;
	}

//takes an individual chunk from a 'struct mcmap_region,' returns a parsed root 'nbt_tag'
struct nbt_tag *mcmap_chunk_read(struct mcmap_region_chunk *c)
	{
	nbt_compression_type t;
	if (c == NULL || c->header == NULL)
		return NULL;
	switch (c->header->compression)
		{
		case 0x1: t=NBT_COMPRESS_GZIP; break;
		case 0x2: t=NBT_COMPRESS_ZLIB; break;
		default: t=NBT_COMPRESS_UNKNOWN; break;
		}
	return nbt_decode(c->data,c->size,t);
	}
