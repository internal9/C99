#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#define HASHMAP_INIT_SIZE 16
#define HASHMAP_NOVALUE (void*) -2	// Allow for keys that store a value of 0

struct Bucket {
	const char *key;
	size_t key_len;
	void *pvalue;
};

struct HashMap {
	struct Bucket *buckets;
	size_t size, stored;
};

void hashmap_put(struct HashMap *pmap, const char *key, size_t key_len, void *pvalue);
void *hashmap_get(struct HashMap *pmap, const char *key, size_t key_len);
bool hashmap_delete(struct HashMap *pmap, const char *key, size_t key_len);

#endif
