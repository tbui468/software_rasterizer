#ifndef GKAB_ARENA_H
#define GKAB_ARENA_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_CHUNK_SIZE 1024

#define TEST

#ifdef TEST
size_t g_bytes_allocated;
size_t g_bytes_freed;
#endif

union gkab_align {
    int i;
    long l;
    long *lp;
    void *p;
    void (*fp)(void);
    float f;
    double d;
    long double ld;
};

static size_t gkab_aligned_size(size_t size) {
    return ((size + sizeof(union gkab_align) - 1) / sizeof(union gkab_align)) * sizeof(union gkab_align);
}

struct gkab_chunk {
    void *base;
    size_t off; 
    size_t size;
};

struct gkab_arena {
    struct gkab_chunk *chunks;
    int chunk_count;
    int chunk_cap;
    int idx;
};

void gkab_chunk_init(struct gkab_chunk *chunk, size_t size) {
#ifdef TEST
    g_bytes_allocated += size;
#endif
    chunk->base = malloc(size);
    chunk->off = 0;
    chunk->size = size;
}

void gkab_chunk_free(struct gkab_chunk *chunk) {
#ifdef TEST
    g_bytes_freed += chunk->size;
#endif
    free(chunk->base);
}

void *gkab_chunk_alloc(struct gkab_chunk *chunk, size_t size) {
    size_t alsize = gkab_aligned_size(size);

    if (chunk->off + alsize > chunk->size) {
        return NULL;
    }

    void *ptr = (char*) chunk->base + chunk->off;
    chunk->off += alsize;
    return ptr;
}

void gkab_arena_init(struct gkab_arena *arena) {
    arena->chunk_cap = 2;
    arena->chunks = malloc(sizeof(struct gkab_chunk) * arena->chunk_cap);
#ifdef TEST
    g_bytes_allocated += sizeof(struct gkab_chunk) * arena->chunk_cap;
#endif
    arena->chunk_count = 1;
    gkab_chunk_init(&arena->chunks[0], DEFAULT_CHUNK_SIZE);
    arena->idx = 0;
}

void gkab_arena_free(struct gkab_arena *arena) {
    size_t freed = 0;
    size_t cap = 0;
    for (int i = 0; i < arena->chunk_count; i++) {
        struct gkab_chunk *c = &arena->chunks[i];
        freed += c->off;
        cap += c->size; 
        gkab_chunk_free(c);
    }
    free(arena->chunks);
#ifdef TEST
    g_bytes_freed += sizeof(struct gkab_chunk) * arena->chunk_cap;
#endif
}

void gkab_arena_reset(struct gkab_arena *arena) {
    for (int i = 0; i < arena->chunk_count; i++) {
        struct gkab_chunk *c = &arena->chunks[i];
        c->off = 0;
    }
    arena->idx = 0;
}

static void gkab_arena_append_chunk(struct gkab_arena *arena, struct gkab_chunk *chunk) {
    if (arena->chunk_count >= arena->chunk_cap) {
        arena->chunk_cap *= 2;
        arena->chunks = realloc(arena->chunks, sizeof(struct gkab_chunk) * arena->chunk_cap);
#ifdef TEST
        g_bytes_allocated += sizeof(struct gkab_chunk) * (arena->chunk_cap / 2);
#endif
    }

    arena->chunks[arena->chunk_count] = *chunk;
    arena->chunk_count++;
}

void* gkab_arena_malloc(struct gkab_arena *arena, size_t size) {
    size_t alsize = gkab_aligned_size(size);

    void *ptr = NULL;
    while (arena->idx < arena->chunk_count) {
        struct gkab_chunk *cur = &arena->chunks[arena->idx];
        ptr = gkab_chunk_alloc(cur, alsize);
        if (!ptr) {
            arena->idx++;
        } else {
            return ptr;
        }
    }

    if (!ptr) {
        size_t new_size = DEFAULT_CHUNK_SIZE;
        while (new_size < alsize) {
            new_size *= 8;
        }
        struct gkab_chunk new_chunk;
        gkab_chunk_init(&new_chunk, new_size);
        ptr = gkab_chunk_alloc(&new_chunk, alsize);
        gkab_arena_append_chunk(arena, &new_chunk);
        assert(ptr);
    }

    return ptr;
}

void* gkab_arena_calloc(struct gkab_arena *arena, size_t nmem, size_t size) {
    size_t alsize = gkab_aligned_size(nmem * size);

    void *ptr = gkab_arena_malloc(arena, alsize);
    memset(ptr, 0, alsize);
    return ptr;
}

void* gkab_arena_realloc(struct gkab_arena *arena, void *ptr, size_t size) {
    size_t alsize = gkab_aligned_size(size);

    size_t copy_len = 0;
    bool found = false; //used to test assert
    if (ptr) {
        for (int i = 0; i < arena->chunk_count; i++) {
            struct gkab_chunk *c = &arena->chunks[i];
            //found chunk of first allocation
            if (ptr >= c->base && ptr < (void*) ((char*) c->base + c->off)) {
                size_t ptr_off = (char*) ptr - (char*) c->base;
                assert(c->off > ptr_off);
                copy_len = c->off - ptr_off;
                copy_len = copy_len > alsize ? alsize : copy_len; //copy length will be smaller size
                found = true;
                break;
            }
        }
    }
    assert(!ptr || found);

    void *new_ptr = gkab_arena_malloc(arena, alsize);
    if (ptr) {
        memcpy(new_ptr, ptr, copy_len);
    }

    return new_ptr;
}

#endif //GKAB_ARENA_H

