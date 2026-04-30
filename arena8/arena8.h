#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

struct Arena {
    void *top_ptr;
    struct Block *current_block;
    size_t current_block_capacity;
    size_t current_block_used;
    size_t default_block_capacity;
};

void arena_init(struct Arena *restrict p_arena, size_t default_block_capacity);
void *arena_alloc(struct Arena *p_arena, size_t amount);
void *arena_realloc(struct Arena *p_arena, void *ptr, ptrdiff_t old_amount, ptrdiff_t new_amount, ptrdiff_t align);
void *arena_realloc_top(struct Arena *p_arena, void *ptr, ptrdiff_t new_amount, ptrdiff_t align); /* grow in place */
void arena_clear(struct Arena *p_arena); /* clear all blocks, effectively making arena unusable */
void arena_reset(struct Arena *p_arena); /* clear until first block */
#endif
