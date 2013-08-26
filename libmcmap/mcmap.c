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
		{
		snprintf(reg_name, MCMAP_MAXNAME, "%s", path);
		reg_name[i-1] = '\0';
		snprintf(reg_name, MCMAP_MAXNAME, "%s/region/r.%d.%d.mca", reg_name, ix, iz);
		}
	else
		snprintf(reg_name, MCMAP_MAXNAME, "%s/region/r.%d.%d.mca", path, ix, iz);
	//open file...
	if ((reg_file = fopen(reg_name,"r")) == NULL)
		return NULL;
	//determine filesize...
	if (fstat(fileno(reg_file),&reg_stat) != 0)
		{
		fprintf(stderr,"libmcmap ERROR: fstat() on \'%s\' returned with error %d\n",reg_name,errno);
		return NULL;
		}
	//allocate buffer...
	if ((buff = (uint8_t *)calloc(reg_stat.st_size,1)) == NULL)
		{
		fprintf(stderr,"libmcmap ERROR: out of memory!\n");
		return NULL;
		}
	//copy file to buffer...
	if ((i = fread(buff,1,reg_stat.st_size,reg_file)) != reg_stat.st_size)
		{
		fprintf(stderr,"libmcmap ERROR: fread() tried to read %d bytes, but did %d bytes\n",(unsigned int)reg_stat.st_size,i);
		return NULL;
		}
	//don't need this anymore...
	fclose(reg_file);
	
	//allocate navigation structure
	if ((reg = (struct mcmap_region *)calloc(1,sizeof(struct mcmap_region))) == NULL)
		{
		fprintf(stderr,"libmcmap ERROR: out of memory!\n");
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
				reg->chunks[z][x].header = reg->chunks[z][x].header+0x05; //'reg->chunks[z][x].data' now points to a block of 'reg->chunks[z][x].header->length - 1' bytes
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
