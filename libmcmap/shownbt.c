#include "nbt.h"
#include "mcmap.h"
#include <stdio.h>
#include <math.h>

#define DEFWID   16
#define DEFHGT    3
#define STRMAX 2048
#define HELPMSG "print input NBT file as ASCII to stdout\nusage: shownbt [-v|-q] [-w WIDTH] [-oX,Z | -nX,Z | -eX,Z] INFILE ...\n\t-v   show full array data\n\t-q   hide array data\n\t-w   print WIDTH array items per line (default %d)\n\t-o   interpret input files as region data and extract chunk (X,Z) from overworld before dumping as ASCII\n\t-n   same as '-o' but from nether\n\t-e   same as '-o' but from the end\n"

int main(int argc, char **argv)
	{
	struct nbt_tag *t;
	int i,j,v=DEFHGT,w=DEFWID,done;
	int infile[argc], x,z, rx,rz, cx,cz;
	struct mcmap_region *r;
	char ret = '\0';
	char fn[STRMAX];
	//help message
	if (argc<2)
		{
		fprintf(stderr,HELPMSG,DEFWID);
		return 0;
		}
	//parse input
	for (i=1; i<argc; i++)
		{
		if (argv[i][0] == '-')
			{
			infile[i] = 0;
			done = 0;
			for (j=1; !done && argv[i][j] != '\0'; j++)
				{
				switch (argv[i][j])
					{
					case 'v': v=-1; break;
					case 'q': v= 0; break;
					case 'w':
						infile[++i] = 0;
						if (sscanf(argv[i],"%d",&w) != 1)
							{
							fprintf(stderr,"couldn't parse '%s %s'\n",argv[i-1],argv[i]);
							return -1;
							}
						done = 1;
						break;
					case 'o':
					case 'n':
					case 'e':
						ret = argv[i][j];
						if (sscanf(&(argv[i][++j]),"%d,%d",&x,&z) != 2)
							{
							fprintf(stderr,"couldn't parse '%s'\n",argv[i]);
							return -1;
							}
						done = 1;
						break;
					case 'h':
						fprintf(stderr,HELPMSG,DEFWID);
						return 0;
						break;
					case '-': break;
					default:
						fprintf(stderr,"couldn't parse '%s'\n",argv[i]);
						return -1;
						break;
					}
				}
			}
		else
			infile[i] = 1;
		}
	//read and print
	for (i=1; i<argc; i++)
		{
		if (infile[i])
			{
			if (ret != '\0')
				{
				//resolve filename
				for (j=0;argv[i][j+1]!='\0';j++);
				switch (ret)
					{
					case 'o':
						snprintf(fn,STRMAX,"%s%sregion/",argv[i],(argv[i][j]=='/'?"":"/"));
						break;
					case 'n':
						snprintf(fn,STRMAX,"%s%sDIM-1/region/",argv[i],(argv[i][j]=='/'?"":"/"));
						break;
					case 'e':
						snprintf(fn,STRMAX,"%s%sDIM1/region/",argv[i],(argv[i][j]=='/'?"":"/"));
						break;
					default: break;
					}
				//resolve coordinates
				rx = (int)floor(((double)x)/32.0);
				rz = (int)floor(((double)z)/32.0);
				cx = ( (x<0) ? ((x+1)%32+31) : (x%32) );
				cz = ( (z<0) ? ((z+1)%32+31) : (z%32) );
				//read data
				if ((r = mcmap_region_read(rx,rz,fn)) == NULL)
					{
					fprintf(stderr,"%s: %s\n",MCMAP_LIBNAME,mcmap_error);
					return -1;
					}
				if (r->chunks[cz][cx].data == NULL)
					{
					fprintf(stderr,"relative chunk (%d,%d) doesn't exist in %s\n",cx,cz,fn);
					return 0;
					}
				if ((t = nbt_decode(r->chunks[cz][cx].data,r->chunks[cz][cx].size,NBT_COMPRESS_UNKNOWN)) == NULL)
					{
					fprintf(stderr,"%s: %s\n",NBT_LIBNAME,nbt_error);
					return -1;
					}
				//free unnessecary data
				mcmap_region_free(r);
				}
			else
				if ((t = nbt_file_read(argv[i])) == NULL)
					{
					fprintf(stderr,"%s: %s\n",NBT_LIBNAME,nbt_error);
					return -1;
					}
			}
		nbt_print_ascii(stdout,t,v,w);
		}
	return 0;
	}
