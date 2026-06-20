#ifndef GKAB_ARRAY_H
#define GKAB_ARRAY_H

#include <stdbool.h>
#include "gkab_arena.h"
#include "gkab_util.h"

#define DEFINE_ARR_TYPE(name, elem_t) \
    struct name { \
        int capacity; \
        int count; \
        elem_t *values; \
        size_t elem_size; \
        struct gkab_arena *arena; \
    }; \
    void name ## _init(struct name *l, struct gkab_arena *arena); \
    extern inline void name ## _append(struct name *l, elem_t v); \
    void name ## _sort(struct name *l, int (*cmp)(const void*, const void*)); \
    void name ## _clear(struct name *l); \
    extern inline elem_t name ## _pop(struct name *l, int idx); \
    void name ## _shuffle(struct name *l); \
    void name ## _copy(struct name *l, const struct name *other, struct gkab_arena *arena);

#define ARRAY_INIT(l, arena, element_t) \
    l->capacity = 8; \
    l->count = 0; \
    l->values = gkab_arena_malloc(arena, sizeof(element_t) * l->capacity); \
    l->elem_size = sizeof(element_t); \
    assert(l->values); \
    l->arena = arena;

#define ARRAY_APPEND(l, element) \
    if (l->count + 1 > l->capacity) { \
        l->capacity *= 2; \
        l->values = gkab_arena_realloc(l->arena, l->values, sizeof(element) * l->capacity); \
    } \
    l->values[l->count] = element; \
    l->count++;

#define ARRAY_SORT(l, cmp) \
    qsort(l->values, l->count, l->elem_size, cmp);

#define ARRAY_CLEAR(arr) \
    arr->count = 0;

//Will copy 0 bytes but ptr out of range of array.  Is this undefined?
#define ARRAY_POP(arr, element_t, idx) \
    assert(arr->count > 0); \
    element_t v = arr->values[idx]; \
    memcpy(arr->values + idx, arr->values + idx + 1, arr->elem_size * (arr->count - idx - 1)); \
    arr->count--; \
    return v;

#define ARRAY_SHUFFLE(arr, elem_t) \
    for (int i = 0; i < arr->count; i++) { \
        int j = gkab_rand_int(i, arr->count - 1); \
        elem_t temp = arr->values[i]; \
        arr->values[i] = arr->values[j]; \
        arr->values[j] = temp; \
    }

#define ARRAY_COPY(dst, src, arena, name, element_t) \
    name ## _init(dst, arena); \
    dst->capacity = src->capacity; \
    dst->values = gkab_arena_malloc(dst->arena, dst->elem_size * dst->capacity); \
    dst->count = src->count; \
    memcpy(dst->values, src->values, dst->elem_size * dst->count);


/* Use this macro to generate array */
#define GKAB_MAKE_ARRAY(name, elem_t) \
    struct name { \
        int capacity; \
        int count; \
        elem_t *values; \
        size_t elem_size; \
        struct gkab_arena *arena; \
    }; \
    void name ## _init(struct name *l, struct gkab_arena* arena) { ARRAY_INIT(l, arena, elem_t); } \
    inline void name ## _append(struct name *l, elem_t v) { ARRAY_APPEND(l, v); } \
    void name ## _sort(struct name *l, int (*cmp)(const void*, const void*)) { ARRAY_SORT(l, cmp); } \
    void name ## _clear(struct name *l) { ARRAY_CLEAR(l); } \
    inline elem_t name ## _pop(struct name *l, int idx) { ARRAY_POP(l, elem_t, idx); } \
    void name ## _shuffle(struct name *l) { ARRAY_SHUFFLE(l, elem_t); } \
    void name ## _copy(struct name *dst, const struct name *src, struct gkab_arena *arena) { ARRAY_COPY(dst, src, arena, name, elem_t); }


#endif //GKAB_ARRAY_H
