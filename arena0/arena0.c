/* Arena Alloc C90 */
#include "arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct Block {
    struct Block *prev_block;
    void *mem;
};

/* is pow of 2 */
#define IS_ALIGNED_2(value) (((value) & ((value) - 1)) == 0)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define arena_add_block(p_arena, mem_size) \
    do { \
        struct Block *p_block = malloc(sizeof(struct Block)); \
        p_block->mem = malloc(mem_size); \
        p_block->prev_block = p_arena->current_block; \
        p_arena->current_block = p_block; \
        p_arena->current_block_used = 0; \
        p_arena->current_block_size = mem_size; \
    } while (0)

void arena_init(struct Arena *p_arena, ptrdiff_t default_block_size) {
    assert(default_block_size > 0);
    p_arena->default_block_size = default_block_size;
    p_arena->current_block_used = 0;
    p_arena->current_block_size = default_block_size;
    /* C guarantees malloc returns a ptr with strictest alignment, I think */
    p_arena->current_block = malloc(sizeof(struct Block));
    p_arena->current_block->prev_block = NULL;
    p_arena->current_block->mem = malloc(default_block_size);
}

/* Required memory alignment otherwise undefined behavior from misaligned access */
/* 'align' is a pow of 2 */
void *arena_align_alloc(struct Arena *p_arena, ptrdiff_t amount, ptrdiff_t align) {
    assert(amount > 0 && align >= 0);
    assert(IS_ALIGNED_2(align));
    // assert(((size_t) (amount) & ((size_t) (align) - 1)) == 0); /* is amount aligned itself */

    /* Blocks may not fully be used up */
    if (amount > p_arena->current_block_size) {
        arena_add_block(p_arena, amount);
        p_arena->top_ptr = p_arena->current_block->mem;
    }
    else {
        /* Implementation-defined behavior if uintptr_t can't be represened by ptrdiff_t, i think */
        uintptr_t offset = (uintptr_t) p_arena->current_block->mem + p_arena->current_block_used;
        p_arena->top_ptr = (char*) p_arena->current_block->mem + p_arena->current_block_used;
        ptrdiff_t padding = -offset & (align - 1);
        if (p_arena->current_block_used + (amount + padding) > p_arena->current_block_size) {
            arena_add_block(p_arena, p_arena->default_block_size);
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
    if (offset + new_amount > p_arena->current_block_size) {
        /* Accept the block won't be fully used up */
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_size));
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
    offset = (char*) p_arena->top_ptr - (char*) p_arena->current_block->mem;
    old_amount = p_arena->current_block_used - offset;
    if (new_amount <= old_amount) return p_arena->top_ptr;
    if (offset + new_amount > p_arena->current_block_size) {
        void *old_ptr = p_arena->current_block->mem;
        arena_add_block(p_arena, MAX(new_amount, p_arena->default_block_size));
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

