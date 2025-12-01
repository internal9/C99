#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"

#define MAX_LOAD_FACTOR 80

uint64_t fnv1a_hash(const char *str, size_t str_len)
{
    uint64_t hash = 0xcbf29ce484222325;
    static const uint64_t fnvprime = 0x100000001b3;

	for (size_t i = 0; i < str_len; i++)
		hash ^= (uint64_t) str[i], hash *= fnvprime;
	return hash;
}

static void hashmap_resize(struct HashMap *p_hashmap)
{
	struct Bucket *old_buckets = p_hashmap->buckets;
	size_t old_size = p_hashmap->size;
	p_hashmap->size *= 2;
	p_hashmap->buckets = calloc(p_hashmap->size * sizeof(struct Bucket), sizeof(struct Bucket));

	if (p_hashmap->buckets == NULL)
	{
		free(old_buckets);
		perror("Failed to allocate memory for buckets during hashmap resize\n");
		exit(1);
	}

	for (size_t i = 0; i < old_size; i++)
	{
		struct Bucket *old_bucket = old_buckets + i;
		if (old_bucket->key == NULL)
			continue;

		hashmap_put(p_hashmap, old_bucket->key, old_bucket->key_len, old_bucket->pvalue);
	}
	free(old_buckets);
}

void hashmap_put(struct HashMap *p_hashmap, const char *key, size_t key_len, void *pvalue)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	int load_factor = (int) (p_hashmap->stored  * 100 / p_hashmap->size);

	if (load_factor > MAX_LOAD_FACTOR)
		hashmap_resize(p_hashmap);

	for (size_t i = 0; i < p_hashmap->size; i++)
	{
		struct Bucket *bucket = p_hashmap->buckets + ((hash + i) % p_hashmap->size);

		if (bucket->key == NULL)
		{
			bucket->key = key;
			bucket->key_len = key_len;
			bucket->pvalue = pvalue;
			p_hashmap->stored++;
			break;
		}
		else if (strncmp(key, bucket->key, key_len) == 0)
		{
			bucket->pvalue = pvalue;
			break;
		}
	}
}

void *hashmap_get(struct HashMap *p_hashmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket bucket;
	bucket = p_hashmap->buckets[hash % p_hashmap->size];

	// i might remove this, idk maybe branching makes it slower
	if (bucket.key == NULL)
		return NULL;

	if (key_len == bucket.key_len && strncmp(key, bucket.key, key_len) == 0)
		return bucket.pvalue;

	// there must be a collision, some other key hashed to the same index
	for (size_t i = 1; i < p_hashmap->size; i++)
	{
		bucket = p_hashmap->buckets[(hash + i) % p_hashmap->size];	// modulo causes wrap around to (hash % size) - 1
		if (bucket.key == NULL)
			continue;

		if (key_len == bucket.key_len && strncmp(key, bucket.key, key_len) == 0)
			return bucket.pvalue;
	}
	// None found
	return NULL;
}

bool hashmap_delete(struct HashMap *p_hashmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket *bucket;
	bucket = p_hashmap->buckets + (hash % p_hashmap->size);

	if (bucket->key == NULL)
		return false;

	if (strncmp(key, bucket->key, key_len) == 0)
	{
		bucket->key = NULL;	// yaaa lets just leave the other members as garbage lol
		return true;
	}

	// there must be a collision, some other key hashed to the same index
	for (size_t i = 1; i < p_hashmap->size; i++)
	{
		bucket = p_hashmap->buckets + (hash + i) % p_hashmap->size;
		if (bucket->key == NULL)
			continue;
			
		if (strncmp(key, bucket->key, key_len) == 0)
		{
			bucket->key = NULL;
			return true;
		}
	}
	return false;
}

// Calloc error handling is up to the programmer
void init_hashmap(struct HashMap *p_hashmap, size_t init_size)
{
	p_hashmap->buckets = calloc(init_size * sizeof(struct Bucket),  sizeof(struct Bucket));
	p_hashmap->size = init_size;
}
