// ?: probably simplify put and get functions in 'hashmap.c'
// bucket values of either 'int' or 'void *'

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#define HASHMAP_INIT_SIZE 16

struct Bucket {
	const char *key;
	size_t key_len;
        union Value {
                void *ptr_v;
                int int_v;
        } value;
};

struct HashMap {
	struct Bucket *buckets;
	size_t size, stored;
};

bool hashmap_put_ptr(struct HashMap *p_hashmap, const char *key, size_t key_len, void *ptr_v);
bool hashmap_put_int(struct HashMap *p_hashmap, const char *key, size_t key_len, int int_v);
void *hashmap_get_ptr(struct HashMap *p_hashmap, const char *key, size_t key_len);
int hashmap_get_int(struct HashMap *p_hashmap, const char *key, size_t key_len);
bool hashmap_delete(struct HashMap *p_hashmap, const char *key, size_t key_len);
bool hashmap_init(struct HashMap *p_hashmap, size_t init_size);
void hashmap_free(struct HashMap *p_hashmap);
#endif
