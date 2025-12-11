#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"

#define MAX_LOAD_FACTOR 80
#define DELETED (void*) -1

uint64_t fnv1a_hash(const char *str, size_t str_len)
{
    uint64_t hash = 0xcbf29ce484222325;
    static const uint64_t fnvprime = 0x100000001b3;

	for (size_t i = 0; i < str_len; i++)
		hash ^= (uint64_t) str[i], hash *= fnvprime;
	return hash;
}

static bool hashmap_resize(struct HashMap *pmap)
{
	struct Bucket *old_buckets = pmap->buckets;
	size_t old_size = pmap->size;
	pmap->size *= 2;
	pmap->buckets = calloc(pmap->size * sizeof(struct Bucket), sizeof(struct Bucket));

	// pointers may be stored as values, so let user manually free them before further freeing
	if (pmap->buckets == NULL)
		return false;

	for (size_t i = 0; i < old_size; i++)
	{
		struct Bucket *old_bucket = old_buckets + i;
		if (old_bucket->key == NULL || old_bucket->key == DELETED)
			continue;

		hashmap_put(pmap, old_bucket->key, old_bucket->key_len, old_bucket->pvalue);
	}
	free(old_buckets);
}

bool hashmap_put(struct HashMap *pmap, const char *key, size_t key_len, void *pvalue)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	int load_factor = (int) (pmap->stored  * 100 / pmap->size);

	if (load_factor > MAX_LOAD_FACTOR)
		if (!hashmap_resize(pmap))
			return false;

	for (size_t i = 0; i < pmap->size; i++)
	{
		struct Bucket *bucket = pmap->buckets + ((hash + i) % pmap->size);

		if (bucket->key == NULL || bucket->key == DELETED)
		{
			bucket->key = key;
			bucket->key_len = key_len;
			bucket->pvalue = pvalue;
			pmap->stored++;
			break;
		}
		else if (strncmp(key, bucket->key, key_len) == 0)
		{
			bucket->pvalue = pvalue;
			break;
		}
	}
	return true;
}

void *hashmap_get(struct HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket bucket;
	bucket = pmap->buckets[hash % pmap->size];

	// i might remove this, idk maybe branching makes it slower
	if (bucket.key == NULL)
		return HASHMAP_NOVALUE;

	if (bucket.key != DELETED && key_len == bucket.key_len &&
	  strncmp(key, bucket.key, key_len) == 0)
		return bucket.pvalue;

	// there must be a collision, some other key hashed to the same index
	for (size_t i = 1; i < pmap->size; i++)
	{
		bucket = pmap->buckets[(hash + i) % pmap->size];	// modulo causes wrap around to (hash % size) - 1
		if (bucket.key == NULL || bucket.key == DELETED)
			continue;

		if (key_len == bucket.key_len && strncmp(key, bucket.key, key_len) == 0)
			return bucket.pvalue;
	}
	return HASHMAP_NOVALUE;	
}

bool hashmap_delete(struct HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket *bucket;
	bucket = pmap->buckets + (hash % pmap->size);

	if (bucket->key == NULL)
		return false;

	if (bucket->key != DELETED && strncmp(key, bucket->key, key_len) == 0)
	{
		bucket->key = DELETED;	// yaaa lets just leave the other members as garbage lol
		return true;
	}

	// there must be a collision, some other key hashed to the same index
	for (size_t i = 1; i < pmap->size; i++)
	{
		bucket = pmap->buckets + (hash + i) % pmap->size;
		if (bucket->key == NULL || bucket->key == DELETED)
			continue;
			
		if (strncmp(key, bucket->key, key_len) == 0)
		{
			bucket->key = DELETED;
			return true;
		}
	}
	return false;
}

bool hashmap_init(struct HashMap *pmap, size_t init_size)
{
	pmap->buckets = calloc(init_size * sizeof(struct bucket), sizeof(struct Bucket));
	if (pmap->buckets == NULL)
		return false;

	pmap->size = init_size;
	return true;
}

inline void hashmap_free(struct HashMap *pmap)
{
	free(pmap->buckets);
}

// TODO: test it lol
int main(void)
{
	struct HashMap map = {
		.buckets = calloc((size_t) HASHMAP_INIT_SIZE * sizeof(struct Bucket), sizeof(struct Bucket)),
		.size = (size_t) HASHMAP_INIT_SIZE
	}, *pmap = &map;

	if (map.buckets == NULL)
	{
		perror("Failed to allocate memory for hashmap buckets");
		return 1;
	}

	// TESTING
	#define KEY_COUNT 1000
	char *strs[KEY_COUNT];

	// trying out this arena alloc or smthn
	char *keys_mem = malloc(8 * KEY_COUNT), *current = keys_mem;
	for (int i = 0; i < KEY_COUNT; i++)
	{
		char *key = current;
		current += 8;
		key[7] = '\0';
		strncpy(key, "KEY", 3);
		for (int num = i, ii = 3; ii >= 0; ii--)
		{
			*(key + 3 + ii) = (int8_t) (num % 10) + '0';
			num /= 10;
		}
		// printf("str: %s\n", key);
		strs[i] = key;

		hashmap_put(pmap, key, 7, (void*) 23);
	}
	printf("HASHMAP SIZE: %zu\n", pmap->size);
	
	for (int i = 0; i < KEY_COUNT; i++)
		if (hashmap_get(pmap, strs[i], 7) == HASHMAP_NOVALUE)
			printf("NULL\n");

	free(keys_mem);
	return 0;
}
