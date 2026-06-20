#ifndef ARRAY_H
#define ARRAY_H

#include "gkab_arena.h"
#include "gkab_array.h"

GKAB_MAKE_ARRAY(vec4_array, vec4s);
GKAB_MAKE_ARRAY(vec3_array, vec3s);
GKAB_MAKE_ARRAY(vec2_array, vec2s);
GKAB_MAKE_ARRAY(iarray, int);

struct indices {
    struct iarray positions;
    struct iarray texcoords;    
    struct iarray normals;    
};

void indices_copy(struct indices *dst, const struct indices *src, struct gkab_arena *arena) {
    iarray_copy(&dst->positions, &src->positions, arena);
    iarray_copy(&dst->texcoords, &src->texcoords, arena);
    iarray_copy(&dst->normals, &src->normals, arena);
}

#endif //ARRAY_H
