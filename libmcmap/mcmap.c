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
	unsigned int x,z,i;
	
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
	for (z=0;z<32;z++)
		{
		for (x=0;x<32;x++)
			{
			if (reg->header->locations[z][x].sector_count > 0)
				{
				reg->locations[z][x] = (unsigned int)( (((uint32_t)(reg->header->locations[z][x].offset[0]))<<16) + (((uint32_t)(reg->header->locations[z][x].offset[1]))<<8) + ((uint32_t)(reg->header->locations[z][x].offset[2])) ); //extract big-endian 24-bit integer from reg->header->location[z][x].offset
				reg->chunks[z][x].header = (struct mcmap_reg_chunkhdr *)&(buff[reg->locations[z][x]*4096]); //connect 5-byte chunk header
				reg->chunks[z][x].size = (unsigned int)( (((uint32_t)(reg->chunks[z][x].header->length[0]))<<24) + (((uint32_t)(reg->chunks[z][x].header->length[1]))<<16) + (((uint32_t)(reg->chunks[z][x].header->length[2]))<<8) + ((uint32_t)(reg->chunks[z][x].header->length[3])) ); //extract big-endian 32-bit integer from reg->chunks[z][x].header->length
				reg->chunks[z][x].data = (uint8_t *)(reg->chunks[z][x].header+0x05); //'reg->chunks[z][x].data' now points to a block of 'reg->chunks[z][x].size - 1' bytes
				}
			}
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
