#include <stdio.h>
#include <stdlib.h>

#define MEMDB_LIBNAME "memdb"
#define MEMDB_MAX 8192

struct memdb_lot
	{
	void *ptr;
	size_t size;
	} memdb_alloced[MEMDB_MAX];
int memdb_first = 0;

void memdb_init(void)
	{
	int i;
	if (!memdb_first)
		{
		memdb_first = 1;
		for (i=0;i<MEMDB_MAX;i++)
			{
			memdb_alloced[i].ptr = NULL;
			memdb_alloced[i].size = 0;
			}
		}
	return;
	}

int memdb_find(void *p)
	{
	int i;
	for (i=0; i<MEMDB_MAX && memdb_alloced[i].ptr != p; i++);
	if (i == MEMDB_MAX)
		{
		if (p == NULL)
			fprintf(stderr,"%s: out of slots for debugging info\n",MEMDB_LIBNAME);
		else
			fprintf(stderr,"%s: %p not found in list of allocated pointers\n",MEMDB_LIBNAME,p);
		exit(EXIT_FAILURE);
		}
	return i;
	}

//return size of memory allocated at 'p', or -1 if not found
int memdb_check(void *p)
	{
	int i;
	memdb_init();
	if (p == NULL)
		return -1;
	for (i=0; i<MEMDB_MAX && memdb_alloced[i].ptr != p; i++);
	if (i == MEMDB_MAX)
		return -1;
	return memdb_alloced[i].size;
	}

//return total memory usage
int memdb_heap_size(void)
	{
	int i,size = 0;
	memdb_init();
	for (i=0;i<MEMDB_MAX;i++)
		if (memdb_alloced[i].ptr != NULL) size += memdb_alloced[i].size;
	return size;
	}

//return total number of allocations
int memdb_heap_num(void)
	{
	int i, count = 0;
	memdb_init();
	for (i=0;i<MEMDB_MAX;i++)
		if (memdb_alloced[i].ptr != NULL) count++;
	return count;
	}

void *memdb_malloc(size_t size)
	{
	void *p;
	int i;
	memdb_init();
	i = memdb_find(NULL);
	if ((p = malloc(size)) == NULL)
		{
		fprintf(stderr,"%s: malloc() returned NULL",MEMDB_LIBNAME);
		exit(EXIT_FAILURE);
		}
	memdb_alloced[i].ptr = p;
	memdb_alloced[i].size = size;
	return p;
	}

void *memdb_calloc(size_t count, size_t size)
	{
	void *p;
	int i;
	memdb_init();
	i = memdb_find(NULL);
	if ((p = calloc(count,size)) == NULL)
		{
		fprintf(stderr,"%s: calloc() returned NULL",MEMDB_LIBNAME);
		exit(EXIT_FAILURE);
		}
	memdb_alloced[i].ptr = p;
	memdb_alloced[i].size = size*count;
	return p;
	}

void *memdb_realloc(void *ptr, size_t size)
	{
	void *p;
	int i;
	memdb_init();
	i = memdb_find(ptr);
	if ((p = realloc(ptr,size)) == NULL)
		{
		fprintf(stderr,"%s: realloc() returned NULL",MEMDB_LIBNAME);
		exit(EXIT_FAILURE);
		}
	memdb_alloced[i].ptr = p;
	memdb_alloced[i].size = size;
	return p;
	}

void memdb_free(void *ptr)
	{
	int i;
	memdb_init();
	i = memdb_find(ptr);
	memdb_alloced[i].ptr = NULL;
	memdb_alloced[i].size = 0;
	free(ptr);
	return;
	}
