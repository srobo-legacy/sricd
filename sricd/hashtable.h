#ifndef _INCLUDED_HASHTABLE
#define _INCLUDED_HASHTABLE

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*hashtable_hash)(const void*);
typedef int (*hashtable_cmp)(const void*, const void*);

typedef struct _hashtable hashtable;

hashtable* hashtable_create(int objsize,
                            int slots,
                            hashtable_hash hash_function,
                            hashtable_cmp compare_function);
void hashtable_destroy(hashtable* table);

void hashtable_insert(hashtable* table, const void* value);
const void* hashtable_find(hashtable* table, const void* key);
void hashtable_delete(hashtable* table, const void* key);

#ifdef __cplusplus
}
#endif

#endif
