#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "mcmap.h"

//searches the given path to a minecraft map folder and parses the region file for the given X & Z region coordinates
//returns pointer to region memory structure; if no file returns NULL
struct mcmap_region *mcmap_read_region(int ix, int iz, char *path)
	{
	FILE *reg_file;
	char reg_name[MCMAP_MAXNAME];
	struct stat reg_stat;
	uint8_t *buff;
	struct mcmap_region *reg;
	unsigned int x,z,i,err;
	
	//resolve filename from map directory...
	for (i=0;path[i]!='\0';i++);
	if (path[i-1] == '/')
		snprintf(reg_name, MCMAP_MAXNAME, "%sregion/r.%d.%d.mca", path, ix, iz);
	else
		snprintf(reg_name, MCMAP_MAXNAME, "%s/region/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((reg_file = fopen(reg_name,"r")) == NULL)
		{
		fprintf(stderr,"%s ERROR: fopen() on \'%s\' returned:\n\t%s\n",MCMAP_LIBNAME,reg_name,strerror(errno));
		return NULL;
		}
	//determine filesize...
	if (fstat(fileno(reg_file),&reg_stat) != 0)
		{
		fprintf(stderr,"%s ERROR: fstat() on \'%s\' returned:\n\t%s\n",MCMAP_LIBNAME,reg_name,strerror(errno));
		return NULL;
		}
	//allocate buffer...
	if ((buff = (uint8_t *)calloc(reg_stat.st_size,1)) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",MCMAP_LIBNAME);
		return NULL;
		}
	//copy file to buffer...
	if ((i = fread(buff,1,reg_stat.st_size,reg_file)) != reg_stat.st_size)
		{
		fprintf(stderr,"%s ERROR: fread() encountered %s on the last %d requested bytes of \'%s\'\n",MCMAP_LIBNAME,(ferror(reg_file)?"an error":"EOF"),(unsigned int)reg_stat.st_size-i,reg_name);
		return NULL;
		}
	//don't need this anymore...
	fclose(reg_file);
	
	//allocate navigation structure
	if ((reg = (struct mcmap_region *)calloc(1,sizeof(struct mcmap_region))) == NULL)
		{
		fprintf(stderr,"%s ERROR: calloc() returned NULL\n",MCMAP_LIBNAME);
		return NULL;
		}
	
	//engage structure to memory space
	reg->header = (struct mcmap_reg_header *)buff;
	err = 0;
	for (z=0;z<32 && !err;z++)
		{
		for (x=0;x<32 && !err;x++)
			{
			if (reg->header->locations[z][x].sector_count > 0)
				{
				//extract big-endian 24-bit integer from reg->header->location[z][x].offset
				reg->locations[z][x] = (unsigned int)( (((uint32_t)(reg->header->locations[z][x].offset[0]))<<16) + (((uint32_t)(reg->header->locations[z][x].offset[1]))<<8) + ((uint32_t)(reg->header->locations[z][x].offset[2])) );
				
				//chunk listing should not point anywhere in the file header
				if (reg->locations[z][x] < 2)
					err = 1;
				//chunk listing should not point past the end of the file
				i = reg->locations[z][x]*4096;
				if (i+reg->header->locations[z][x].sector_count*4096 >= reg_stat.st_size)
					err = 1;
				
				if (!err)
					{
					//connect 5-byte chunk header
					reg->chunks[z][x].header = (struct mcmap_reg_chunkhdr *)&(buff[i]);
					//extract big-endian 32-bit integer from reg->chunks[z][x].header->length (same location as buff[i])
					reg->chunks[z][x].size = (unsigned int)( (((uint32_t)(buff[i]))<<24) + (((uint32_t)(buff[i+1]))<<16) + (((uint32_t)(buff[i+2]))<<8) + ((uint32_t)(buff[i+3])) ) - 1;
					//'reg->chunks[z][x].data' will now point to a block of 'reg->chunks[z][x].size' bytes
					reg->chunks[z][x].data = (uint8_t *)(reg->chunks[z][x].header+0x05);
					
					//listed chunk size should not be larger than the rest of the file
					if (i+5+reg->chunks[z][x].size >= reg_stat.st_size)
						err = 1;
					//in fact neither should it be larger than the sector count in the file header
					if (reg->chunks[z][x].size+5 > reg->header->locations[z][x].sector_count*4096)
						err = 1;
					}
				}
			else
				{
				reg->chunks[z][x].header = NULL;
				reg->chunks[z][x].size = 0;
				reg->chunks[z][x].data = NULL;
				}
			}
		}
	if (err)
		{
		fprintf(stderr,"%s ERROR: file \'%s\' may be correupted\n",MCMAP_LIBNAME,reg_name);
		return NULL;
		}
	
	return reg;
	}

//free all memory allocated in mcmap_read_region()
void mcmap_free_region(struct mcmap_region *reg)
	{
	free(reg->header); //free the memory buffer of the file contents
	free(reg); //free the navigation nodes that were connected throughout the buffer
	return;
	}
