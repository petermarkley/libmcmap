#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <errno.h>
#include "../nbt.h"
#include "mcmap.h"

//searches the given path to a minecraft map folder and parses the region file for the given X & Z region coordinates
struct mcmap_region *mcmap_read_region(int x, int z, char *path)
	{
	FILE *reg_file;
	char reg_name[MCMAP_MAXNAME];
	unsigned char buff[16];
	struct mcmap_region *reg;
	unsigned int i;
	
	//open file...
	snprintf(reg_name, MCMAP_MAXNAME, "%s/region/r.%d.%d.mca", path, x, z);
	if ((reg_file = fopen(reg_name,"r")) == NULL)
		{
		//fprintf(stderr,"mcmap error: %d\n",errno);
		return NULL;
		}
	
	//allocate region structure...
	if ((reg = (struct mcmap_region *)calloc(1,sizeof(struct mcmap_region))) == NULL)
		{
		fprintf(stderr,"mcmap error: out of memory\n");
		return NULL;
		}
	
	//allocate chunks...
	if ((reg->chunks = (unsigned char ***)calloc(32,sizeof(unsigned char **))) == NULL)
		{
		fprintf(stderr,"mcmap error: out of memory\n");
		return NULL;
		}
	for (i=0; i<32; i++)
		{
		if ((reg->chunks[i] = (unsigned char **)calloc(32,sizeof(unsigned char *))) == NULL)
			{
			fprintf(stderr,"mcmap error: out of memory\n");
			return NULL;
			}
		}
	
	//read chunks...
	for (z=0; z<32; z++)
		{
		for (x=0; x<32; x++)
			{
			fseek(reg_file, (x + z*32)*4, SEEK_SET);
			fread(buff,1,4,reg_file);
			if (buff[3] != 0x00) //if chunk exists...
				{
				i = (((unsigned int)buff[0])<<16) + (((unsigned int)buff[1])<<8) + (unsigned int)buff[2]; //read location
				fseek(reg_file, i*4096, SEEK_SET); //seek to location
				fread(buff,1,5,reg_file); //read chunk header
				reg->compression_type[z][x] = buff[4]; //read compression type
				if (buff[4] != 0x02)
					fprintf(stderr,"warning: unknown compression type %x for chunk %d, %d\n", buff[4], x, z);
				i = (((unsigned int)buff[0])<<24) + (((unsigned int)buff[1])<<16) + (((unsigned int)buff[2])<<8) + (unsigned int)buff[3]; //read chunk length
				
				//allocate chunk...
				if ((reg->chunks[z][x] = (unsigned char *)malloc(i-1)) == NULL)
					{
					fprintf(stderr,"mcmap error: out of memory\n");
					return NULL;
					}
				fread(reg->chunks[z][x],1,i-1,reg_file); //read chunk
				reg->lengths[z][x] = i-1; //record length
				}
			}
		}
	
	//read timestamps...
	for (z=0; z<32; z++)
		{
		for (x=0; x<32; x++)
			{
			fseek(reg_file, (x + z*32)*4+4096, SEEK_SET);
			fread(buff,1,4,reg_file);
			reg->dates[z][x] = (((unsigned int)buff[0])<<24) + (((unsigned int)buff[1])<<16) + (((unsigned int)buff[2])<<8) + (unsigned int)buff[3];
			}
		}
	
	fclose(reg_file);
	return reg;
	}

//free region memory structure
void mcmap_free_region(struct mcmap_region *reg)
	{
	int x, z;
	if (reg != NULL)
		{
		for (z=0; z<32; z++)
			{
			for (x=0; x<32; x++)
				{
				if (reg->chunks[z][x] != NULL)
					free(reg->chunks[z][x]);
				}
			free(reg->chunks[z]);
			}
		free(reg->chunks);
		}
	free(reg);
	return;
	}

struct mcmap_chunk *mcmap_read_chunk(int cx, int cz, struct mcmap_region *reg)
	{
	int bx,by,bz, y_inx, index, block_found, data_found;
	struct mcmap_chunk *chunk;
	struct nbt_node *trunk, *level, *sections, *secnode, *blocknode;
	const struct nbt_list *entry;
	struct list_head *posa, *posb, *posc;
	unsigned int bytebuff[16][16][16];
	unsigned char databuff[16][16][16];
	
	//sanity-check input
	cx = cx%32; cz = cz%32;
	if (cx < 0) cx = 31-cx;
	if (cz < 0) cz = 31-cz;
	if (reg->chunks[cz][cx] == NULL)
		return NULL;
	
	//allocate chunk
	if ((chunk = (struct mcmap_chunk *)calloc(1,sizeof(struct mcmap_chunk))) == NULL)
		{
		fprintf(stderr,"mcmap error: out of memory\n");
		return NULL;
		}
	//initialize blocks at 0
	for (by=0; by<256; by++)
		{
		for (bz=0; bz<16; bz++)
			{
			for (bx=0; bx<16; bx++)
				{
				chunk->blocks[by][bz][bx] = 0;
				chunk->data  [by][bz][bx] = 0;
				if (by == 0) chunk->biomes[bz][bx] = MCMAP_NOBIOME;
				}
			}
		}
	
	//parse NBT data (thanks Lukas Niederbremer & Clark Gaebel!)
	if ((trunk = nbt_parse_compressed(reg->chunks[cz][cx],reg->lengths[cz][cx])) == NULL)
		{
		fprintf(stderr,"mcmap error: %s\n",nbt_error_to_string(errno));
		return NULL;
		}
	
	//descend past the root tag
	if (trunk->type != TAG_COMPOUND)
		{
		fprintf(stderr,"mcmap error: malformed chunk\n");
		return NULL;
		}
	entry = list_entry(trunk->payload.tag_compound->entry.flink, const struct nbt_list, entry);
	level = entry->data;
	//check the next tag
	if (strcmp(level->name,"Level") != 0 || level->type != TAG_COMPOUND)
		{
		fprintf(stderr,"mcmap error: malformed chunk\n");
		return NULL;
		}
	//scan for section list
	list_for_each(posa, &level->payload.tag_compound->entry)
		{
		entry = list_entry(posa, const struct nbt_list, entry);
		sections = entry->data;
		if (strcmp(sections->name,"Biomes") == 0 && sections->type == TAG_BYTE_ARRAY)
			{
			if (sections->payload.tag_byte_array.length != 256)
				{
				fprintf(stderr,"mcmap error: malformed chunk\n");
				return NULL;
				}
			for (bz=0; bz<16; bz++)
				{
				for (bx=0; bx<16; bx++)
					chunk->biomes[bz][bx] = sections->payload.tag_byte_array.data[bx+bz*16];
				}
			}
		else if (strcmp(sections->name,"Sections") == 0 && sections->type == TAG_LIST && sections->payload.tag_list->data != NULL && sections->payload.tag_list->data->type == TAG_COMPOUND)
			{
			//loop through each section
			list_for_each(posb, &sections->payload.tag_list->entry)
				{
				entry = list_entry(posb, const struct nbt_list, entry);
				secnode = entry->data;
				if (secnode->type != TAG_COMPOUND)
					{
					fprintf(stderr,"mcmap error: malformed chunk\n");
					return NULL;
					}
				//initialize buffer
				for (by=0; by<16; by++)
					{
					for (bz=0; bz<16; bz++)
						{
						for (bx=0; bx<16; bx++)
							bytebuff[by][bz][bx] = 0;
						}
					}
				//scan section for relevant data
				y_inx = -1; block_found = data_found = 0;
				list_for_each(posc, &secnode->payload.tag_compound->entry)
					{
					entry = list_entry(posc, const struct nbt_list, entry);
					blocknode = entry->data;
					//save section index
					if (strcmp(blocknode->name,"Y") == 0 && blocknode->type == TAG_BYTE)
						{
						if (blocknode->payload.tag_byte < 16 && blocknode->payload.tag_byte >= 0)
							y_inx = (int)blocknode->payload.tag_byte;
						else
							{
							fprintf(stderr,"mcmap error: malformed chunk\n");
							return NULL;
							}
						}
					//parse extended block data
					else if (strcmp(blocknode->name,"Add") == 0 && blocknode->type == TAG_BYTE_ARRAY)
						{
						if (blocknode->payload.tag_byte_array.length != 2048)
							{
							fprintf(stderr,"mcmap error: malformed chunk\n");
							return NULL;
							}
						for (by=0; by<16; by++)
							{
							for (bz=0; bz<16; bz++)
								{
								for (bx=0; bx<16; bx++)
									{
									index = bx+bz*16+by*256;
									if (index%2)
										bytebuff[by][bz][bx] += (unsigned int)((blocknode->payload.tag_byte_array.data[(int)(index/2)]&0xF0)<<4);
									else
										bytebuff[by][bz][bx] += (unsigned int)((blocknode->payload.tag_byte_array.data[(int)(index/2)]&0x0F)<<8);
									}
								}
							}
						}
					//parse base block data
					else if (strcmp(blocknode->name,"Blocks") == 0 && blocknode->type == TAG_BYTE_ARRAY)
						{
						block_found = 1;
						if (blocknode->payload.tag_byte_array.length != 4096)
							{
							fprintf(stderr,"mcmap error: malformed chunk\n");
							return NULL;
							}
						for (by=0; by<16; by++)
							{
							for (bz=0; bz<16; bz++)
								{
								for (bx=0; bx<16; bx++)
									bytebuff[by][bz][bx] += (unsigned int)blocknode->payload.tag_byte_array.data[bx+bz*16+by*256];
								}
							}
						}
					//parse extra block data
					else if (strcmp(blocknode->name,"Data") == 0 && blocknode->type == TAG_BYTE_ARRAY)
						{
						data_found = 1;
						if (blocknode->payload.tag_byte_array.length != 2048)
							{
							fprintf(stderr,"mcmap error: malformed chunk\n");
							return NULL;
							}
						for (by=0; by<16; by++)
							{
							for (bz=0; bz<16; bz++)
								{
								for (bx=0; bx<16; bx++)
									{
									index = bx+bz*16+by*256;
									if (index%2)
										databuff[by][bz][bx] = (blocknode->payload.tag_byte_array.data[(int)(index/2)]&0xF0)>>4;
									else
										databuff[by][bz][bx] = (blocknode->payload.tag_byte_array.data[(int)(index/2)]&0x0F);
									}
								}
							}
						}
					}
				if (y_inx == -1 || !block_found || !data_found)
					{
					fprintf(stderr,"mcmap error: malformed chunk\n");
					return NULL;
					}
				//now that we have all the needed info, populate chunk
				for (by=0; by<16; by++)
					{
					for (bz=0; bz<16; bz++)
						{
						for (bx=0; bx<16; bx++)
							{
							chunk->blocks[by+y_inx*16][bz][bx] = bytebuff[by][bz][bx];
							chunk->data  [by+y_inx*16][bz][bx] = databuff[by][bz][bx];
							}
						}
					}
				}
			}
		}
	
	nbt_free(trunk);
	return chunk;
	}
