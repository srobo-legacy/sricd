#ifndef _INCLUDED_POOL
#define _INCLUDED_POOL

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _pool pool;

pool* pool_create(unsigned objsize);
void pool_destroy(pool* pl);

void* pool_alloc(pool* pl);
void pool_free(pool* pl, void* ptr);

#ifdef __cplusplus
}
#endif

#endif
