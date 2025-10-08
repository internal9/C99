#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define INITIAL_SIZE 16
#define MAX_LOAD_FACTOR 7
#define EMPTY (void*) 0	// NULL is implementation defined
#define DELETED (void*) -1
#define NOVALUE (void*) -2	// Allow for keys that store values of 0

// should probably make prototypes
typedef struct {
	const char *key;
	size_t key_len;
	void *pvalue;
} Bucket;

typedef struct {
	Bucket *buckets;
	size_t size, stored;
} HashMap;

uint64_t fnv1a_hash(const char *str, size_t str_len)
{
    uint64_t hash = 0xcbf29ce484222325;
    static const uint64_t fnvprime = 0x100000001b3;
    
	for (size_t i = 0; i < str_len; i ++)
		hash ^= (uint64_t) str[i], hash *= fnvprime;
	return hash;
}

void hashmap_resize(HashMap *pmap)
{
	Bucket *old_buckets = pmap->buckets;
	size_t old_size = pmap->size;
	pmap->size *= 2;
	pmap->buckets = calloc(pmap->size * sizeof(Bucket), sizeof(Bucket));

	if (pmap->buckets == NULL)
	{
		free(old_buckets);
		perror("Failed to allocate memory for buckets during hashmap resize\n");
		exit(1);
	}

	for (size_t i = 0; i < old_size; i++)
	{
		Bucket bucket = old_buckets[i];
		if (bucket.key == EMPTY || bucket.key == DELETED)
			continue;

		uint64_t hash = fnv1a_hash(bucket.key, bucket.key_len);
		pmap->buckets[(hash) % pmap->size] = bucket;
	}
	free(old_buckets);
}

void hashmap_put(HashMap *pmap, const char *key, size_t key_len, void *pvalue)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	int load_factor = (int) (pmap->stored * 100 / pmap->size);

	// printf("LOAD FACTOR: %d\n", load_factor);
	if (load_factor > MAX_LOAD_FACTOR)
		hashmap_resize(pmap);

	for (size_t i = 0; i < pmap->size; i++)
	{
		Bucket *bucket = pmap->buckets + ((hash + i) % pmap->size);

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

void *hashmap_get(HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	Bucket bucket;
	bucket = pmap->buckets[hash % pmap->size];

	// i might remove this, idk maybe branching makes it slower
	if (bucket.key == EMPTY)
		return NOVALUE;

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
	return NOVALUE;	// idk if i should use null, since it's implementation defined
}

bool hashmap_delete(HashMap *pmap, const char *key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	Bucket *bucket;
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
	HashMap map = {
		.buckets = calloc((size_t) INITIAL_SIZE * sizeof(Bucket),  sizeof(Bucket)),
		.size = (size_t) INITIAL_SIZE
	};

	if (map.buckets == NULL)
	{
		perror("Failed to allocate memory for hashmap buckets");
		return 1;
	}

	HashMap *pmap = &map;

	int asd = 23;
	hashmap_put(pmap, "1234567890", 10, (void*) &asd);
	asd = 23333;

	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	hashmap_resize(pmap);
	
	printf("%d\n", * (int*) hashmap_get(pmap, "1234567890", 10));
	return 0;
}
