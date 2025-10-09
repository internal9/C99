#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"

#define MAX_LOAD_FACTOR 7
#define EMPTY (void*) 0	// NULL is implementation defined
#define DELETED (void*) -1
#define HASHMAP_NOVALUE (void*) -2	// Allow for keys that store values of 0

uint64_t fnv1a_hash(const char *str, size_t str_len)
{
    uint64_t hash = 0xcbf29ce484222325;
    static const uint64_t FNV_PRIME = 0x100000001b3;
    
	for (size_t i = 0; i < str_len; i ++)
		hash ^= (uint64_t) str[i], hash *= FNV_PRIME;
	return hash;
}

static void hashmap_resize(struct HashMap *pmap)
{
	struct Bucket *old_buckets = pmap->buckets;
	size_t old_size = pmap->size;
	pmap->size *= 2;
	pmap->buckets = calloc(pmap->size * sizeof(struct Bucket), sizeof(struct Bucket));

	if (pmap->buckets == NULL)
	{
		free(old_buckets);
		perror("Failed to allocate memory for buckets during hashmap resize\n");
		exit(1);
	}

	for (size_t i = 0; i < old_size; i++)
	{
		struct Bucket bucket = old_buckets[i];
		if (bucket.key == EMPTY || bucket.key == DELETED)
			continue;

		uint64_t hash = fnv1a_hash(bucket.key, bucket.key_len);
		pmap->buckets[(hash) % pmap->size] = bucket;
	}
	free(old_buckets);
}

void hashmap_put(struct HashMap *pmap, const char *key, size_t key_len, void *pvalue)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	int load_factor = (int) (pmap->stored * 100 / pmap->size);

	// printf("LOAD FACTOR: %d\n", load_factor);
	if (load_factor > MAX_LOAD_FACTOR)
		hashmap_resize(pmap);

	for (size_t i = 0; i < pmap->size; i++)
	{
		struct Bucket *bucket = pmap->buckets + ((hash + i) % pmap->size);

		if (bucket->key == EMPTY || bucket->key == DELETED)
		{
			bucket->key = key;
			bucket->key_len = key_len;
			bucket->pvalue = pvalue;
			pmap->stored++;
			break;
		}
		else if (strncmp(key, bucket->key, key_len))
		{
			bucket->pvalue = pvalue;
			pmap->stored++;
			break;
		}
	}
}

void *hashmap_get(struct HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket bucket;
	bucket = pmap->buckets[hash % pmap->size];

	// i might remove this, idk maybe branching makes it slower
	if (bucket.key == EMPTY)
		return HASHMAP_NOVALUE;

	if (bucket.key != DELETED && strncmp(key, bucket.key, key_len) == 0)
		return bucket.pvalue;

	// there must be a collision, some other key hashed to the same index
	for (size_t i = 1; i < pmap->size; i++)
	{
		bucket = pmap->buckets[(hash + i) % pmap->size];	// modulo causes wrap around to (hash % size) - 1
		if (bucket.key == EMPTY || bucket.key == DELETED)
			continue;
	
		if (strncmp(key, bucket.key, key_len) == 0)
			return bucket.pvalue;
	}
	printf("Get: No specified key found!\n");
	return HASHMAP_NOVALUE;	// idk if i should use null, since it's implementation defined
}

bool hashmap_delete(struct HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket *bucket;
	bucket = pmap->buckets + (hash % pmap->size);

	if (bucket->key == EMPTY)
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
		if (bucket->key == EMPTY || bucket->key == DELETED)
			continue;
			
		if (strncmp(key, bucket->key, key_len) == 0)
		{
			bucket->key = DELETED;
			return true;
		}
	}
	return false;
}

// TODO: test it lol
int main(void)
{
	struct HashMap map = {
		.buckets = calloc((size_t) HASHMAP_INIT_SIZE * sizeof(struct Bucket),  sizeof(struct Bucket)),
		.size = (size_t) HASHMAP_INIT_SIZE
	}, *pmap = &map;

	if (map.buckets == NULL)
	{
		perror("Failed to allocate memory for hashmap buckets");
		return 1;
	}

/*	int asd = 23;
	hashmap_put(pmap, "1234567890", 10, (void*) &asd);
	asd = 23333;
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_put(pmap, "56", 2, NULL);
	hashmap_resize(pmap);
	hashmap_put(pmap, "56", 2, NULL);
	hashmap_put(pmap, "256", 3, NULL);
	hashmap_put(pmap, "456", 3, NULL);
		hashmap_resize(pmap);
	hashmap_resize(pmap);

	hashmap_put(pmap, "516", 3, NULL);
	hashmap_put(pmap, "556", 3, NULL);
	hashmap_put(pmap, "576", 3, NULL);

		hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);

	hashmap_put(pmap, "5b6", 3, NULL);
	hashmap_put(pmap, "5c6", 3, NULL);

		hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	long asdb = 23345;

	hashmap_put(pmap, "5zz6", 4, &asdb);

		hashmap_resize(pmap);
	hashmap_resize(pmap);

	hashmap_put(pmap, "5ll6", 4, NULL);
	hashmap_put(pmap, "5pp6", 4, NULL);

	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	
	printf("%zu\n", pmap->stored); */
	return 0;
}
