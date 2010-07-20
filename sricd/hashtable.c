#include "hashtable.h"
#include "pool.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct _hash_entry hash_entry;

struct _hash_entry {
	hash_entry* chain;
	unsigned char value[0];
};

struct _hashtable {
	hashtable_hash hash_function;
	hashtable_cmp cmp_function;
	pool* pool;
	int tablesize, objsize;
	hash_entry* entries[];
};

static int hash_index(hashtable* table, const void* value)
{
	int hash, index;
	hash = table->hash_function(value);
	index = hash % index;
	return index;
}

hashtable* hashtable_create(int objsize,
                            int slots,
                            hashtable_hash hash_function,
                            hashtable_cmp compare_function)
{
	int i;
	pool* p;
	hashtable* ht;
	p = pool_create_extra(sizeof(hash_entry) + objsize,
	                      sizeof(hashtable) + sizeof(hash_entry*)*slots,
	                      (void**)&ht);
	ht->hash_function = hash_function;
	ht->cmp_function = compare_function;
	ht->pool = pool_create(sizeof(hash_entry) + objsize);
	ht->tablesize = slots;
	ht->objsize = objsize;
	for (i = 0; i < slots; ++i) {
		ht->entries[i] = NULL;
	}
	return ht;
}

void hashtable_destroy(hashtable* table)
{
	assert(table);
	pool_destroy(table->pool);
}

void hashtable_insert(hashtable* table, const void* value)
{
	hash_entry** entry;
	int index;
	assert(table);
	assert(value);
	index = hash_index(table, value);
	entry = &(table->entries[index]);
	while (*entry) {
		if (!table->cmp_function((*entry)->value, value)) {
			// key match, copy in and be done
			memcpy((*entry)->value, value, table->objsize);
		}
		entry = &((*entry)->chain);
	}
	*entry = pool_alloc(table->pool);
	(*entry)->chain = NULL;
	memcpy((*entry)->value, value, table->objsize);
}

const void* hashtable_find(hashtable* table, const void* key)
{
	hash_entry* entry;
	int index;
	assert(table);
	assert(key);
	index = hash_index(table, key);
	entry = table->entries[index];
	while (entry && table->cmp_function(entry->value, key)) {
		entry = entry->chain;
	}
	if (entry)
		return entry->value;
	else
		return NULL;
}

void hashtable_delete(hashtable* table, const void* key)
{
	hash_entry* entry;
	hash_entry* prev = NULL;
	int index;
	assert(table);
	assert(key);
	index = hash_index(table, key);
	entry = table->entries[index];
	while (entry && table->cmp_function(entry->value, key)) {
		prev = entry;
		entry = entry->chain;
	}
	if (entry) {
		if (prev) {
			prev->chain = entry->chain;
		} else {
			table->entries[index] = entry->chain;
		}
		pool_free(table->pool, entry);
	}
}
