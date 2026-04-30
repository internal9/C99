#ifndef ARENA_H
#define ARENA_H
#define ARENA_ALIGNOF(data_type) offsetof(struct {char _; data_type placeholder;}, placeholder)

struct Arena {
    void *top_ptr;
    struct Block *current_block;
    ptrdiff_t current_block_size;
    ptrdiff_t current_block_used;
    ptrdiff_t default_block_size;
};

struct Arena *arena_new(ptrdiff_t init_size);
void *arena_align_alloc(struct Arena *p_arena, ptrdiff_t amount, ptrdiff_t align);
/* all caps just to signify it's a macro and not an 'inline' function */
#define ARENA_TYPE_ALLOC(p_arena, element_type) arena_align_alloc(p_arena, sizeof(element_type), ARENA_ALIGNOF(element_type));
/* !!! realloc assumes same align */
void *arena_realloc(struct Arena *p_arena, void *ptr, ptrdiff_t old_amount, ptrdiff_t new_amount); /* no ptr param, as it's known at top */
void *arena_realloc_top(struct Arena *p_arena, ptrdiff_t new_amount);
void arena_clear(struct Arena *p_arena); /* clear all blocks, effectively making arena unusable */
void arena_reset(struct Arena *p_arena); /* clear until first block */

#undef DEFAULT_ALIGNMENT
#endif
