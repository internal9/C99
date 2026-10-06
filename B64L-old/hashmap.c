// fix get functions not working properly sometimes
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

static struct Bucket *find_bucket(struct HashMap *p_hashmap, const char* key, size_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket *p_bucket;

	for (size_t i = 0; i < p_hashmap->size; i++)
	{
		p_bucket = p_hashmap->buckets + ((hash + i) % p_hashmap->size);	// modulo causes wrap around to (hash % size) - 1
		if (p_bucket->key == NULL)
			continue;

		if (key_len == p_bucket->key_len && strncmp(key, p_bucket->key, key_len) == 0)
			return p_bucket;
	}
	// None found
	return NULL;        
}

static struct Bucket *find_empty_or_matching_bucket(struct HashMap *p_hashmap,
                                                    const char *key, uint64_t key_len)
{
	uint64_t hash = fnv1a_hash(key, key_len);
	struct Bucket *p_bucket;

	for (size_t i = 0; i < p_hashmap->size; i++)
	{
                p_bucket = p_hashmap->buckets + ((hash + i) % p_hashmap->size);
		if (p_bucket->key == NULL || strncmp(key, p_bucket->key, key_len) == 0)
                        return p_bucket;
	}

        return NULL;
}

static bool hashmap_resize(struct HashMap *p_hashmap)
{
	struct Bucket *old_buckets = p_hashmap->buckets;
	size_t old_size = p_hashmap->size;
	p_hashmap->size *= 2;
	p_hashmap->buckets = calloc(p_hashmap->size * sizeof(struct Bucket), sizeof(struct Bucket));

        // failed to allocate memory for new buckets
	if (p_hashmap->buckets == NULL)
	{
		free(old_buckets);
		return false;
	}

	for (size_t i = 0; i < old_size; i++)
	{
		struct Bucket *old_bucket = old_buckets + i;
		if (old_bucket->key == NULL)
			continue;

                struct Bucket *p_bucket = find_empty_or_matching_bucket(p_hashmap,
                                                                        old_bucket->key,
                                                                        old_bucket->key_len);
                if (p_bucket->key == NULL) {
                        p_bucket->key = old_bucket->key;
                        p_bucket->key_len = old_bucket->key_len;
                        p_bucket->value = old_bucket->value;
                }
	}

	free(old_buckets);
	return true;
}

bool hashmap_put_ptr(struct HashMap *p_hashmap, const char *key, size_t key_len, void *ptr_v)
{
	int load_factor = (int) (p_hashmap->stored * 100 / p_hashmap->size);
	if (load_factor > MAX_LOAD_FACTOR)
		if (!hashmap_resize(p_hashmap))
                        return false;

        struct Bucket *p_bucket = find_empty_or_matching_bucket(p_hashmap, key,
                                                                key_len);
        p_bucket->value.ptr_v = ptr_v;
        if (p_bucket->key == NULL) {
                p_bucket->key = key;
                p_bucket->key_len = key_len;
                p_hashmap->stored++;
        }

        return true;
}

bool hashmap_put_int(struct HashMap *p_hashmap, const char *key, size_t key_len, int int_v)
{
	int load_factor = (int) (p_hashmap->stored * 100 / p_hashmap->size);
	if (load_factor > MAX_LOAD_FACTOR)
		if (!hashmap_resize(p_hashmap))
                        return false;

        struct Bucket *p_bucket = find_empty_or_matching_bucket(p_hashmap, key,
                                                                key_len);
        p_bucket->value.int_v = int_v;
        if (p_bucket->key == NULL) {
                p_bucket->key = key;
                p_bucket->key_len = key_len;
                p_hashmap->stored++;
        }

        return true;
}

void *hashmap_get_ptr(struct HashMap *p_hashmap, const char *key, size_t key_len)
{
	struct Bucket *p_bucket = find_bucket(p_hashmap, key, key_len);
        return p_bucket != NULL ? p_bucket->value.ptr_v : NULL;
}

// returns '-1' if key not found
int hashmap_get_int(struct HashMap *p_hashmap, const char *key, size_t key_len)
{
        struct Bucket *p_bucket = find_bucket(p_hashmap, key, key_len);
        return p_bucket != NULL ? p_bucket->value.int_v  : -1;
}

// Returns a bool indicating if delete succeeded
bool hashmap_delete(struct HashMap *p_hashmap, const char *key, size_t key_len)
{
        struct Bucket *p_bucket = find_bucket(p_hashmap, key, key_len);
        if (p_bucket == NULL)
                return false;

        p_bucket->key = NULL;
        p_bucket->key_len = 0;
        p_hashmap->stored--;
        return true;
}

// Calloc error handling is up to the programmer
bool hashmap_init(struct HashMap *p_hashmap, size_t init_size)
{
	p_hashmap->buckets = calloc(init_size * sizeof(struct Bucket), sizeof(struct Bucket));
	p_hashmap->size = init_size;
	return p_hashmap->buckets != NULL;
}

// Bruh
void hashmap_free(struct HashMap *p_hashmap)
{
        free(p_hashmap->buckets);
}
