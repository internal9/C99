/*
 * NOTES: bc of a default alignment of 8, fuction pointers *may*
 * not be supported since they may have a an alignment of 16
 */
#include "arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// TBD: flexible array member to replace mallocing for 'mem'?
struct Block {
    struct Block *prev_block;
    // void *mem;
    char mem[];
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
// (v + (align - 1)) & ~(align - 1)
#define ALIGN8(unsigned_value) (((unsigned_value) + 7) & -7)

static inline void arena_add_block(struct Arena *restrict p_arena, size_t mem_capacity) {
    mem_capacity = MAX(ALIGN8(mem_capacity), p_arena->default_block_capacity);
    struct Block *p_block = malloc(sizeof(struct Block) + mem_capacity);
    // p_block->mem = malloc(mem_capacity);
    p_block->prev_block = p_arena->current_block;
    p_arena->current_block = p_block;
    p_arena->current_block_used = 0;
    p_arena->current_block_capacity = mem_capacity;
}

void arena_init(struct Arena *restrict p_arena, size_t default_block_capacity) {
    default_block_capacity = ALIGN8(default_block_capacity);
    p_arena->default_block_capacity = default_block_capacity;
    p_arena->current_block_used = 0;
    p_arena->current_block_capacity = default_block_capacity;
    /* C guarantees malloc returns a ptr with strictest alignment, I think */
    p_arena->current_block = malloc(sizeof(struct Block));
    p_arena->current_block->prev_block = NULL;
    p_arena->current_block->mem = malloc(default_block_capacity);
}

/* Required memory alignment otherwise undefined behavior from misaligned access */
/* 'align' is a pow of 2 */
void *arena_alloc(struct Arena *p_arena, ptrdiff_t amount) {
    assert(amount > 0 && align >= 0);
    
    // assert(((size_t) (amount) & ((size_t) (align) - 1)) == 0); /* is amount aligned itself */

    /* Blocks may not fully be used up */
    if (amount > p_arena->current_block_capacity) {
        arena_add_block(p_arena, amount);
        p_arena->top_ptr = p_arena->current_block->mem;
    }
    else {
        /* Implementation-defined behavior if uintptr_t can't be represened by ptrdiff_t, i think */
        uintptr_t offset = (uintptr_t) p_arena->current_block->mem + p_arena->current_block_used;
        p_arena->top_ptr = (char*) p_arena->current_block->mem + p_arena->current_block_used;
        ptrdiff_t padding = -offset & (align - 1);
        if (p_arena->current_block_used + (amount + padding) > p_arena->current_block_capacity) {
            arena_add_block(p_arena, p_arena->default_block_capacity);
            p_arena->top_ptr = p_arena->current_block->mem;
        }
        else {
            amount += padding;
            p_arena->top_ptr += padding;
        }
    }

    p_arena->current_block_used += amount;
    /* printf("%llu\n", p_arena->current_block_used); */
    return (void*) p_arena->top_ptr;
}

void *arena_realloc(struct Arena *p_arena, void *ptr, ptrdiff_t old_amount, ptrdiff_t new_amount) {
    ptrdiff_t offset;
    if (new_amount <= old_amount) return ptr;
    offset = (char*) ptr - (char*) p_arena->current_block->mem;
    if (offset + new_amount > p_arena->current_block_capacity) {
        /* Accept the block won't be fully used up */
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_capacity));
        p_arena->current_block_used = new_amount;
        p_arena->top_ptr = memcpy(p_arena->current_block->mem, ptr, old_amount);
    }
    else
        /* Grow in same block, as it's at top */
        p_arena->current_block_used += new_amount - old_amount;
    return p_arena->top_ptr;
}

/* no ptr param, as it's known at top */
void *arena_realloc_top(struct Arena *p_arena, ptrdiff_t new_amount) {
    ptrdiff_t offset, old_amount;
    assert(new_amount > 0);
    assert(p_arena->top_ptr != NULL);
    offset = (char*) p_arena->top_ptr - (char*) p_arena->current_block->mem;
    old_amount = p_arena->current_block_used - offset;
    if (new_amount <= old_amount) return p_arena->top_ptr;
    if (offset + new_amount > p_arena->current_block_capacity) {
        void *old_ptr = p_arena->current_block->mem;
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_capacity));
        p_arena->current_block_used = new_amount;
        p_arena->top_ptr = memcpy(p_arena->current_block->mem, p_arena->top_ptr, old_amount);
    } else
        /* Grow in same block */
        p_arena->current_block_used += new_amount - old_amount;
    return p_arena->top_ptr;
}

void arena_reset(struct Arena *p_arena) {
    struct Block *p_block = p_arena->current_block;
    struct Block *p_prev_block = p_block->prev_block;
    while (p_prev_block != NULL) {
        free(p_block->mem);
        free(p_block);
        p_block = p_prev_block;
        p_prev_block = p_block->prev_block;
    }
    p_arena->current_block = p_block;
    p_arena->current_block_used = p_arena->default_block_capacity;
    p_arena->current_block_used = 0;
    p_arena->top_ptr = NULL;
}

void arena_clear(struct Arena *p_arena) {
    struct Block *p_block = p_arena->current_block;
    struct Block *p_prev_block;
    while (p_block != NULL) {
        p_prev_block = p_block->prev_block;
        free(p_block->mem);
        free(p_block);
        p_block = p_prev_block;
    }
}
