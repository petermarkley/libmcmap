#ifndef __MEMDB_HEADER
#define __MEMDB_HEADER

#undef malloc
#undef calloc
#undef realloc
#undef free

void *memdb_malloc(size_t size);
void *memdb_calloc(size_t count, size_t size);
void *memdb_realloc(void *ptr, size_t size);
#define malloc(size) memdb_malloc((size))
#define calloc(count,size) memdb_calloc((count),(size))
#define realloc(ptr,size) memdb_realloc((ptr),(size))

void memdb_free(void *ptr);
#define free(ptr) memdb_free((ptr))

//return size of memory allocated at 'p', or -1 if not found
int memdb_check(void *p);

#endif
