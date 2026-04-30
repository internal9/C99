/*
 * NOTES: bc of a default alignment of 8, fuction pointers *may*
 * not be supported since they may have a an alignment of 16
 */
#include "arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct Block {
    struct Block *prev_block;
    char mem[];
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
// (v + (align - 1)) & ~(align - 1), align forwards
#define ALIGN8(unsigned_value) (((unsigned_value) + 7) & ~7)

static inline void arena_add_block(struct Arena *restrict p_arena, size_t block_capacity) {
    struct Block *p_block = malloc(sizeof(struct Block) + block_capacity);
    p_block->prev_block = p_arena->current_block;
    p_arena->current_block = p_block;
    p_arena->current_block_used = 0;
    p_arena->current_block_capacity = block_capacity;
}

void arena_init(struct Arena *restrict p_arena, size_t default_block_capacity) {
    assert(default_block_capacity > 0);
    default_block_capacity = ALIGN8(default_block_capacity);
    p_arena->default_block_capacity = default_block_capacity;
    p_arena->current_block_used = 0;
    p_arena->current_block_capacity = default_block_capacity;
    /* C guarantees malloc returns a ptr with strictest alignment, I think */
    p_arena->current_block = malloc(sizeof(struct Block) + default_block_capacity);
    p_arena->current_block->prev_block = NULL;
    p_arena->top_ptr = NULL;
}

// Required memory alignment otherwise undefined behavior from misaligned access
void *arena_alloc(struct Arena *restrict p_arena, size_t amount) {
    assert(amount > 0);
    amount = ALIGN8(amount); // just align size, so pointers themselves are already aligned
    // Blocks may not fully be used up
    if (p_arena->current_block_used + amount > p_arena->current_block_capacity) {
        arena_add_block(p_arena, amount);
        p_arena->top_ptr = p_arena->current_block->mem;
    }
    else
        p_arena->top_ptr = p_arena->current_block->mem + p_arena->current_block_used;
    p_arena->current_block_used += amount;
    return p_arena->top_ptr;
}

void *arena_realloc(struct Arena *p_arena, void *ptr, size_t old_amount, size_t new_amount) {
    if (new_amount <= old_amount) return ptr;
    size_t offset = (char*) ptr - (char*) p_arena->current_block->mem;
    // Accept the block won't be fully used up
    // Unable to know next ptr if ptr isn't at top, so yeah
    if (p_arena->top_ptr != ptr || offset + new_amount > p_arena->current_block_capacity) {
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_capacity));
        p_arena->current_block_used = new_amount;
        p_arena->top_ptr = memcpy(p_arena->current_block->mem, ptr, old_amount);
    }
    else
        // Grow in same block, as it's at top
        p_arena->current_block_used += new_amount - old_amount;
    return p_arena->top_ptr;
}

// No ptr param, as it's known at top
void *arena_realloc_top(struct Arena *restrict p_arena, size_t new_amount) {
    assert(new_amount > 0);
    assert(p_arena->top_ptr != NULL);
    size_t offset = (char*) p_arena->top_ptr - (char*) p_arena->current_block->mem;
    size_t old_amount = p_arena->current_block_used - offset;
    if (new_amount <= old_amount) return p_arena->top_ptr;
    if (offset + new_amount > p_arena->current_block_capacity) {
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_capacity));
        p_arena->current_block_used = new_amount;
        p_arena->top_ptr = memcpy(p_arena->current_block->mem, p_arena->top_ptr, old_amount);
    } else
        // Grow in same block
        p_arena->current_block_used += new_amount - old_amount;
    return p_arena->top_ptr;
}

void arena_reset(struct Arena *restrict p_arena) {
    struct Block *p_block = p_arena->current_block;
    struct Block *p_prev_block = p_block->prev_block;
    while (p_prev_block != NULL) {
        free(p_block);
        p_block = p_prev_block;
        p_prev_block = p_block->prev_block;
    }
    p_arena->current_block = p_block;
    p_arena->current_block_capacity = p_arena->default_block_capacity;
    p_arena->current_block_used = 0;
    p_arena->top_ptr = NULL;
}

void arena_clear(const struct Arena *restrict p_arena) {
    struct Block *p_block = p_arena->current_block;
    struct Block *p_prev_block;
    while (p_block != NULL) {
        p_prev_block = p_block->prev_block;
        free(p_block);
        p_block = p_prev_block;
    }
}
