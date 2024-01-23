#include "nbt.h"
#include "mcmap.h"
#include <stdio.h>
#include <math.h>

#define DEFWID   16
#define DEFHGT    3
#define STRMAX 2048
#define HELPMSG "print input NBT file as ASCII to stdout\nusage: shownbt [-v|-q] [-w WIDTH] [-oX,Z | -nX,Z | -eX,Z] [-p PATH] INFILE ...\n\t-v   show full array data\n\t-q   hide array data\n\t-w   print WIDTH array items per line (default %d)\n\t-o   interpret input files as region data and extract chunk (X,Z) from overworld before dumping as ASCII\n\t-n   same as '-o' but from nether\n\t-e   same as '-o' but from the end\n\t-p   provide PATH to tag inside file, a series of tag names separated by forward slashes '/'\n"

int main(int argc, char **argv)
	{
	struct nbt_tag *t, *probe, *parent;
	int i,j,k,l,m,v=DEFHGT,w=DEFWID,done;
	int infile=-1, x,z, rx,rz, cx,cz;
	struct mcmap_region *r;
	char ret = '\0';
	char fn[STRMAX];
	char *path = NULL;
	int path_size, ongoing;
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
			done = 0;
			for (j=1; !done && argv[i][j] != '\0'; j++)
				{
				switch (argv[i][j])
					{
					case 'v': v=-1; break;
					case 'q': v= 0; break;
					case 'w':
						i++;
						if (i >= argc)
							{
							fprintf(stderr,"missing argument after '-w'\n");
							return -1;
							}
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
					case 'p':
						i++;
						if (i >= argc)
							{
							fprintf(stderr,"missing argument after '-p'\n");
							return -1;
							}
						//count number of elements in series
						ongoing = 0;
						path_size = 0;
						for (k=0; argv[i][k] != '\0'; k++)
							{
							if (argv[i][k] == '/')
								ongoing = 0;
							else if (!ongoing)
								{
								ongoing = 1;
								path_size++;
								}
							}
						//allocate list of tag names
						if ((path = (char *)calloc(path_size,sizeof(char)*STRMAX)) == NULL)
							{
							fprintf(stderr,"calloc() returned NULL\n");
							return -1;
							}
						//parse list of tag names
						ongoing = 0;
						l = 0;
						m = 0;
						for (k=0; argv[i][k] != '\0' && k < STRMAX-1 && l < path_size; k++)
							{
							if (argv[i][k] == '/')
								{
								if (ongoing)
									{
									path[l*STRMAX+m] = '\0';
									l++;
									m = 0;
									}
								ongoing = 0;
								}
							else
								{
								ongoing = 1;
								path[l*STRMAX+m] = argv[i][k];
								m++;
								}
							}
						if (ongoing && k < STRMAX)
							path[l*STRMAX+m] = '\0';
						done = 1;
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
			infile = i;
		}
	//read and print
	i = infile;
	if (i > 0)
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
		if (path == NULL)
			nbt_print_ascii(stdout,t,v,w);
		else
			{
			parent = t;
			for (k=0; k < path_size; k++)
				{
				if ((probe = nbt_child_find(parent,NBT_END,&(path[k*STRMAX]))) == NULL)
					{
					fprintf(stderr,"couldn't find tag '%s' inside '%s'\n",&(path[k*STRMAX]),parent->name);
					return -1;
					}
				parent = probe;
				}
			nbt_print_ascii(stdout,parent,v,w);
			}
		nbt_free(t);
		if (path != NULL)
			free(path);
		}
	return 0;
	}
