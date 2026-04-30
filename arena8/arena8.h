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
void *arena_alloc(struct Arena *restrict p_arena, size_t amount);
void *arena_realloc(struct Arena *restrict p_arena, void *ptr, size_t old_amount, size_t new_amount);
void *arena_realloc_top(struct Arena *restrict p_arena, size_t new_amount); // grow in place
void arena_reset(struct Arena *restrict p_arena); // clear until first block
void arena_clear(const struct Arena *restrict p_arena); // clear all blocks, effectively making arena unusable
#endif
