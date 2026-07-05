#ifndef RENDERER_H
#define RENDERER_H

#include <assert.h>
#include "cglm/struct.h"
#include "gkab_arena.h"
#include "gkab_array.h"
#include "array.h"
#include "model.h"

struct raster_vertex {
    vec4s pos;
    vec2s tex;
    vec3s world_pos;
    vec3s normal;
    float inv_w;
    struct texture *texture;
};

GKAB_MAKE_ARRAY(clips, struct raster_vertex);

struct renderer {
    float *depths;
    struct gkab_arena arena;
    struct gkab_arena frame_arena;
    struct clips clips;

    uint32_t *pixels;
    int width;
    int height;

    struct vec2_array vec2_array_0;
    struct vec2_array vec2_array_1;
    struct vec3_array vec3_array_0;
    struct vec3_array vec3_array_1;
    struct vec3_array vec3_array_2;
    struct vec3_array vec3_array_3;
    struct vec4_array temps_0;
    struct vec4_array temps_1;
};

void renderer_init(struct renderer *r, uint32_t *pixels, int width, int height) {
    r->pixels = pixels;
    r->width = width;
    r->height = height;
    gkab_arena_init(&r->arena);
    clips_init(&r->clips, &r->arena);

    r->depths = gkab_arena_malloc(&r->arena, sizeof(float) * width * height);

    gkab_arena_init(&r->frame_arena);
    
    vec4_array_init(&r->temps_0, &r->arena);
    vec4_array_init(&r->temps_1, &r->arena);
    vec2_array_init(&r->vec2_array_0, &r->arena);
    vec2_array_init(&r->vec2_array_1, &r->arena);

    vec3_array_init(&r->vec3_array_0, &r->arena);
    vec3_array_init(&r->vec3_array_1, &r->arena);
    vec3_array_init(&r->vec3_array_2, &r->arena);
    vec3_array_init(&r->vec3_array_3, &r->arena);
}

void renderer_begin_frame(struct renderer *r, unsigned char *clear_data) {
    gkab_arena_reset(&r->frame_arena);
    clips_clear(&r->clips);
    memcpy(r->pixels, clear_data, r->width * r->height * 4);

    for (int i = 0; i < r->width * r->height; i++) {
        r->depths[i] = 1.0f;
    }
}

vec4s vec4_interp(vec4s to, vec4s from, float t) {
    return glms_vec4_add(glms_vec4_scale(glms_vec4_sub(to, from), t), from);
}

vec3s vec3_interp(vec3s to, vec3s from, float t) {
    return glms_vec3_add(glms_vec3_scale(glms_vec3_sub(to, from), t), from);
}

vec2s vec2_interp(vec2s to, vec2s from, float t) {
    return glms_vec2_add(glms_vec2_scale(glms_vec2_sub(to, from), t), from);
}

float compute_denom(float x, float y) {
    float d = x - y;
    if (fabsf(d) < 1e-6f) {
        d = (d < 0.f ? -1e-6f : 1e-6f);
    }
    return d;
}

#define MAX_CLIPS 8

struct vec4_clips {
    vec4s buf[MAX_CLIPS];
    int count;
};
void vec4_clips_init(struct vec4_clips *c) {
    c->count = 0;
}
static void inline vec4_clips_append(struct vec4_clips *c, vec4s v) {
    c->buf[c->count] = v;
    c->count++;
}
static void inline vec4_clips_append3(struct vec4_clips *c, const vec4s *v) {
    c->buf[c->count + 0] = *v;
    c->buf[c->count + 1] = *(v + 1);
    c->buf[c->count + 2] = *(v + 2);
    c->count += 3;
}
struct vec3_clips {
    vec3s buf[MAX_CLIPS];
    int count;
};
void vec3_clips_init(struct vec3_clips *c) {
    c->count = 0;
}
static void inline vec3_clips_append(struct vec3_clips *c, vec3s v) {
    c->buf[c->count] = v;
    c->count++;
}
static void inline vec3_clips_append3(struct vec3_clips *c, const vec3s *v) {
    c->buf[c->count + 0] = *v;
    c->buf[c->count + 1] = *(v + 1);
    c->buf[c->count + 2] = *(v + 2);
    c->count += 3;
}
struct vec2_clips {
    vec2s buf[MAX_CLIPS];
    int count;
};
void vec2_clips_init(struct vec2_clips *c) {
    c->count = 0;
}
static void inline vec2_clips_append(struct vec2_clips *c, vec2s v) {
    c->buf[c->count] = v;
    c->count++;
}
static void inline vec2_clips_append3(struct vec2_clips *c, const vec2s *v) {
    c->buf[c->count + 0] = *v;
    c->buf[c->count + 1] = *(v + 1);
    c->buf[c->count + 2] = *(v + 2);
    c->count += 3;
}

static void inline clip_two_vertices(const vec4s *vecs, 
                       const vec2s *uvs, 
                       const vec3s *worlds, 
                       const vec3s *normals, 
                       int internal_vertex_idx, 
                       const float *deltas, 
                       struct vec4_clips *clipped_vertices, 
                       struct vec2_clips *clipped_texcoords,
                       struct vec3_clips *clipped_worldcoords,
                       struct vec3_clips *clipped_normals) {

    for (int i = 0; i < 3; i++) {
        float denom = compute_denom(deltas[i], deltas[internal_vertex_idx]);
        float t = i == internal_vertex_idx ? 0.0f : deltas[i] / denom;

        vec4s intersection = vec4_interp(vecs[internal_vertex_idx], vecs[i], t);
        vec4_clips_append(clipped_vertices, intersection);

        vec2s texcoord_intersection = vec2_interp(uvs[internal_vertex_idx], uvs[i], t);
        vec2_clips_append(clipped_texcoords, texcoord_intersection);

        vec3s worldcoord_intersection = vec3_interp(worlds[internal_vertex_idx], worlds[i], t);
        vec3_clips_append(clipped_worldcoords, worldcoord_intersection);

        vec3s normal_intersection = vec3_interp(normals[internal_vertex_idx], normals[i], t);
        vec3_clips_append(clipped_normals, normal_intersection);
    }
}

static void inline clip_one_vertex(const vec4s *vecs, 
                     const vec2s *uvs, 
                     const vec3s *worlds, 
                     const vec3s *normals, 
                     int external_vertex_idx, 
                     const float *deltas, 
                     struct vec4_clips *clipped_vertices, 
                     struct vec2_clips *clipped_texcoords,
                     struct vec3_clips *clipped_worldcoords,
                     struct vec3_clips *clipped_normals) {
    int next = (external_vertex_idx + 1) % 3;
    int next_next = (external_vertex_idx + 2) % 3;

    float denom0 = compute_denom(deltas[external_vertex_idx], deltas[next]);
    float t0 = deltas[external_vertex_idx] / denom0;
    vec4s i0 = vec4_interp(vecs[next], vecs[external_vertex_idx], t0);
    vec2s tex_i0 = vec2_interp(uvs[next], uvs[external_vertex_idx], t0);
    vec3s world_i0 = vec3_interp(worlds[next], worlds[external_vertex_idx], t0);
    vec3s normal_i0 = vec3_interp(normals[next], normals[external_vertex_idx], t0);

    vec4_clips_append(clipped_vertices, i0);
    vec4_clips_append(clipped_vertices, vecs[next]);
    vec4_clips_append(clipped_vertices, vecs[next_next]);

    vec2_clips_append(clipped_texcoords, tex_i0);
    vec2_clips_append(clipped_texcoords, uvs[next]);
    vec2_clips_append(clipped_texcoords, uvs[next_next]);

    vec3_clips_append(clipped_worldcoords, world_i0);
    vec3_clips_append(clipped_worldcoords, worlds[next]);
    vec3_clips_append(clipped_worldcoords, worlds[next_next]);

    vec3_clips_append(clipped_normals, normal_i0);
    vec3_clips_append(clipped_normals, normals[next]);
    vec3_clips_append(clipped_normals, normals[next_next]);

    float denom1 = compute_denom(deltas[external_vertex_idx], deltas[next_next]);
    float t1 = deltas[external_vertex_idx] / denom1;
    vec4s i1 = vec4_interp(vecs[next_next], vecs[external_vertex_idx], t1);
    vec2s tex_i1 = vec2_interp(uvs[next_next], uvs[external_vertex_idx], t1);
    vec3s world_i1 = vec3_interp(worlds[next_next], worlds[external_vertex_idx], t1);
    vec3s normal_i1 = vec3_interp(normals[next_next], normals[external_vertex_idx], t1);

    vec4_clips_append(clipped_vertices, i0);
    vec4_clips_append(clipped_vertices, vecs[next_next]);
    vec4_clips_append(clipped_vertices, i1);

    vec2_clips_append(clipped_texcoords, tex_i0);
    vec2_clips_append(clipped_texcoords, uvs[next_next]);
    vec2_clips_append(clipped_texcoords, tex_i1);

    vec3_clips_append(clipped_worldcoords, world_i0);
    vec3_clips_append(clipped_worldcoords, worlds[next_next]);
    vec3_clips_append(clipped_worldcoords, world_i1);

    vec3_clips_append(clipped_normals, normal_i0);
    vec3_clips_append(clipped_normals, normals[next_next]);
    vec3_clips_append(clipped_normals, normal_i1);
}

enum clip_plane_type {
    CP_NEAR_Z = 0,
    CP_FAR_Z,
    /*
    CP_POS_X,
    CP_NEG_X,
    CP_POS_Y,
    CP_NEG_Y,
    */
    CP_END
};

static int inline get_clipped_vertices_and_deltas(const vec4s *vecs, enum clip_plane_type type, float *deltas) {
    switch (type) {
    /*
    case CP_POS_X:
        deltas[0] = vecs[0].x - vecs[0].w;
        deltas[1] = vecs[1].x - vecs[1].w;
        deltas[2] = vecs[2].x - vecs[2].w;
        return (vecs[0].x >= vecs[0].w) * 1 + (vecs[1].x >= vecs[1].w) * 2 + (vecs[2].x >= vecs[2].w) * 4;
    case CP_NEG_X:
        deltas[0] = vecs[0].x - -vecs[0].w;
        deltas[1] = vecs[1].x - -vecs[1].w;
        deltas[2] = vecs[2].x - -vecs[2].w;
        return (vecs[0].x <= -vecs[0].w) * 1 + (vecs[1].x <= -vecs[1].w) * 2 + (vecs[2].x <= -vecs[2].w) * 4;
    case CP_POS_Y:
        deltas[0] = vecs[0].y - vecs[0].w;
        deltas[1] = vecs[1].y - vecs[1].w;
        deltas[2] = vecs[2].y - vecs[2].w;
        return (vecs[0].y >= vecs[0].w) * 1 + (vecs[1].y >= vecs[1].w) * 2 + (vecs[2].y >= vecs[2].w) * 4;
    case CP_NEG_Y:
        deltas[0] = vecs[0].y - -vecs[0].w;
        deltas[1] = vecs[1].y - -vecs[1].w;
        deltas[2] = vecs[2].y - -vecs[2].w;
        return (vecs[0].y <= -vecs[0].w) * 1 + (vecs[1].y <= -vecs[1].w) * 2 + (vecs[2].y <= -vecs[2].w) * 4;
    */
    case CP_NEAR_Z: {
        deltas[0] = vecs[0].z - -vecs[0].w;
        deltas[1] = vecs[1].z - -vecs[1].w;
        deltas[2] = vecs[2].z - -vecs[2].w;
        return (vecs[0].w <= 0.0f) * 1 + (vecs[1].w <= 0.0f) * 2 + (vecs[2].w <= 0.0f) * 4;
    }
    case CP_FAR_Z: { //TODO haven't tested this thoroughly
        deltas[0] = vecs[0].z - vecs[0].w;
        deltas[1] = vecs[1].z - vecs[1].w;
        deltas[2] = vecs[2].z - vecs[2].w;
        return (vecs[0].z >= vecs[0].w) * 1 + (vecs[1].z >= vecs[1].w) * 2 + (vecs[2].z >= vecs[2].w) * 4;
    }
    default:
        assert(false);
        return 0;
    }
}

void clip_triangle(struct renderer *r, 
                   const vec4s *vecs, 
                   const vec2s *texcoords, 
                   const vec3s *worldcoords, 
                   const vec3s *normals, 
                   struct vec4_array *clipped_vertices, 
                   struct vec2_array *clipped_texcoords,
                   struct vec3_array *clipped_worldcoords,
                   struct vec3_array *clipped_normals) {

    struct vec4_clips pos_clips_0;
    vec4_clips_init(&pos_clips_0);
    struct vec4_clips pos_clips_1;
    vec4_clips_init(&pos_clips_1);

    struct vec4_clips *cur = &pos_clips_0;
    struct vec4_clips *next = &pos_clips_1;

    struct vec2_clips texcoord_clips_0;
    vec2_clips_init(&texcoord_clips_0);
    struct vec2_clips texcoord_clips_1;
    vec2_clips_init(&texcoord_clips_1);

    struct vec2_clips *cur_texcoords = &texcoord_clips_0;
    struct vec2_clips *next_texcoords = &texcoord_clips_1;

    struct vec3_clips world_clips_0;
    vec3_clips_init(&world_clips_0);
    struct vec3_clips world_clips_1;
    vec3_clips_init(&world_clips_1);

    struct vec3_clips *cur_worldcoords = &world_clips_0;
    struct vec3_clips *next_worldcoords = &world_clips_1;

    struct vec3_clips normal_clips_0;
    vec3_clips_init(&normal_clips_0);
    struct vec3_clips normal_clips_1;
    vec3_clips_init(&normal_clips_1);

    struct vec3_clips *cur_normals = &normal_clips_0;
    struct vec3_clips *next_normals = &normal_clips_1;

    vec4_clips_append3(cur, vecs);
    vec2_clips_append3(cur_texcoords, texcoords);
    vec3_clips_append3(cur_worldcoords, worldcoords);
    vec3_clips_append3(cur_normals, normals);

    for (enum clip_plane_type type = CP_NEAR_Z; type < CP_END; type++) {
        for (int i = 0; i < cur->count; i += 3) {
            vec4s *vecs = cur->buf + i;
            vec2s *uvs = cur_texcoords->buf + i;
            vec3s *worlds = cur_worldcoords->buf + i;
            vec3s *normals = cur_normals->buf + i;

            float d[3];
            int outside_code = get_clipped_vertices_and_deltas(vecs, type, d);

            switch (outside_code) {
            case 1: { //v0 outside
                clip_one_vertex(vecs, uvs, worlds, normals, 0, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            }
            case 2:  //v1 outside
                clip_one_vertex(vecs, uvs, worlds, normals, 1, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            case 4:  //v2 outside
                clip_one_vertex(vecs, uvs, worlds, normals, 2, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            
            case 3: { //v0 and v1 outside
                clip_two_vertices(vecs, uvs, worlds, normals, 2, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            }
            case 5: { //v0 and v2 outside 
                clip_two_vertices(vecs, uvs, worlds, normals, 1, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            }
            case 6: //v1 and v2 outside
                clip_two_vertices(vecs, uvs, worlds, normals, 0, d, next, next_texcoords, next_worldcoords, next_normals);
                break;
            case 7: //all outside plane
                break;
            case 0: //all inside plane
                vec4_clips_append3(next, vecs);
                vec2_clips_append3(next_texcoords, uvs);
                vec3_clips_append3(next_worldcoords, worlds);
                vec3_clips_append3(next_normals, normals);
                break;
            default:
                assert(false);
                break;
            }
        }

        //cleanup arrays and swap for next plane clipping iteration
        struct vec4_clips *temp = cur;
        cur = next;
        next = temp;
        next->count = 0;

        struct vec2_clips *temp1 = cur_texcoords;
        cur_texcoords = next_texcoords;
        next_texcoords = temp1;
        next_texcoords->count = 0;

        struct vec3_clips *temp2 = cur_worldcoords;
        cur_worldcoords = next_worldcoords;
        next_worldcoords = temp2;
        next_worldcoords->count = 0;

        struct vec3_clips *temp3 = cur_normals;
        cur_normals = next_normals;
        next_normals = temp3;
        next_normals->count = 0;
    }

    //end plane iteration loop here
    assert(cur->count == cur_texcoords->count && cur->count == cur_worldcoords->count && cur->count == cur_normals->count);
    for (int i = 0; i < cur->count; i++) {
        vec4_array_append(clipped_vertices, cur->buf[i]);
        vec2_array_append(clipped_texcoords, cur_texcoords->buf[i]);
        vec3_array_append(clipped_worldcoords, cur_worldcoords->buf[i]);
        vec3_array_append(clipped_normals, cur_normals->buf[i]);
    }
}

void clip_triangles(struct renderer *r, 
                        const struct vec4_array *vertices,  //projected vertices
                        const struct vec3_array *world_vertices,
                        const struct vec2_array *texcoords, 
                        const struct vec3_array *normals, 
                        const struct indices *indices,
                        struct texture *texture) {

    struct vec4_array clipped_vertices;
    vec4_array_init(&clipped_vertices, &r->frame_arena);
    struct vec2_array clipped_texcoords;
    vec2_array_init(&clipped_texcoords, &r->frame_arena);
    struct vec3_array clipped_worldcoords;
    vec3_array_init(&clipped_worldcoords, &r->frame_arena);
    struct vec3_array clipped_normals;
    vec3_array_init(&clipped_normals, &r->frame_arena);


    for (int i = 0; i < indices->positions.count; i += 3) {
        vec4s vecs[3];
        vec2s uvs[3];
        vec3s world_pos[3];
        vec3s normal_dir[3];


        for (int j = 0; j < 3; j++) {

            vecs[j] = vertices->values[indices->positions.values[i + j]];
            uvs[j] = indices->texcoords.count == 0 || i + j >= indices->texcoords.count ? 
                        (vec2s) { (float) (j / 2), (float) (j % 2) } : 
                        texcoords->values[indices->texcoords.values[i + j]];
            world_pos[j] = world_vertices->values[indices->positions.values[i + j]];
            normal_dir[j] = indices->normals.count == 0 ? 
                        (vec3s) { 1.0f, 0.0f, 0.0f } : 
                        normals->values[indices->normals.values[i + j]];
        }

        clip_triangle(r, vecs, uvs, world_pos, normal_dir, &clipped_vertices, &clipped_texcoords, &clipped_worldcoords, &clipped_normals);
    }
   
    assert(clipped_vertices.count % 3 == 0 && clipped_vertices.count == clipped_texcoords.count);
    for (int i = 0; i < clipped_vertices.count; i++) {
        vec4s v = clipped_vertices.values[i];
        //normalized device coordinates
        float inv_w = 1.0f / v.w;
        vec4s cv = glms_vec4_scale(v, 1.0f / v.w);

        //viewport transformation
        cv = glms_vec4_add(cv, (vec4s) { 1.0f, 1.0f, 1.0f, 0.0f });
        cv = glms_vec4_scale(cv, 0.5f);
        cv.x *= r->width;
        cv.y *= r->height;
        struct raster_vertex result = { .pos=cv, 
                                        .tex=clipped_texcoords.values[i], 
                                        .inv_w=inv_w, 
                                        .texture=texture,
                                        .world_pos=clipped_worldcoords.values[i],
                                        .normal=clipped_normals.values[i] };
        clips_append(&r->clips, result);
    }
}

void submit_static_mesh(struct renderer *r, 
                        const struct vec3_array *vertices, 
                        const struct vec2_array *texcoords, 
                        const struct vec3_array *normals, 
                        const struct indices *indices,
                        struct texture *texture,
                        mat4s proj_view) {

    assert(indices->positions.count % 3 == 0);

    struct vec4_array projected_vertices;
    vec4_array_init(&projected_vertices, &r->frame_arena);
    struct vec3_array world_vertices;
    vec3_array_init(&world_vertices, &r->frame_arena);

    for (int i = 0; i < vertices->count; i++) {
        vec4s v = { vertices->values[i].x, vertices->values[i].y, vertices->values[i].z, 1.0f };
        vec4_array_append(&projected_vertices, glms_mat4_mulv(proj_view, v));
        assert(false); // not implemented world vertices
    }

    clip_triangles(r, &projected_vertices, &world_vertices, texcoords, normals, indices, texture);
}

void submit_dynamic_mesh(struct renderer *r, 
                         const struct vec3_array *vertices, 
                         const struct vec2_array *texcoords, 
                         const struct vec3_array *normals, 
                         const struct indices *indices, 
                         struct texture *texture,
                         mat4s proj_view, mat4s model, mat4s rotate) {

    assert(indices->positions.count % 3 == 0);

    struct vec4_array projected_vertices;
    vec4_array_init(&projected_vertices, &r->frame_arena);
    struct vec3_array world_vertices;
    vec3_array_init(&world_vertices, &r->frame_arena);
    struct vec3_array transformed_normals;
    vec3_array_init(&transformed_normals, &r->frame_arena);

    for (int i = 0; i < vertices->count; i++) {
        vec4s v = { vertices->values[i].x, vertices->values[i].y, vertices->values[i].z, 1.0f };
        vec4_array_append(&projected_vertices, glms_mat4_mulv(glms_mat4_mul(proj_view, model), v));

        vec4s world_pos = glms_mat4_mulv(model, v);
        vec3_array_append(&world_vertices, (vec3s) { world_pos.x, world_pos.y, world_pos.z });
    }

    for (int i = 0; i < normals->count; i++) {
        vec4s n = { normals->values[i].x, normals->values[i].y, normals->values[i].z, 1.0f };
        vec4s rotated_n = glms_mat4_mulv(rotate, n);
        vec3_array_append(&transformed_normals, (vec3s) { rotated_n.x, rotated_n.y, rotated_n.z });
    }

    clip_triangles(r, &projected_vertices, &world_vertices, texcoords, &transformed_normals, indices, texture);
}

bool line_collides_with_wall(vec3s flashlight_pos, vec3s world_pos, const struct vec4_array *walls) {
    //ambient lighting
    for (int i = 0; i < walls->count; i++) {
        vec4s *v = &walls->values[i]; //x, y, w, h of wall (ignoring z-dimension)
        
        //for each wall edge
        //find intersection (x, y) between spotlight and wall edge 
        //if _any_ wall is intersected (adding small epsilon), break since no need to check any others
        float x0, y0, x1, y1;

        if (flashlight_pos.x < world_pos.x) {
            x0 = flashlight_pos.x;
            y0 = flashlight_pos.y; 
            x1 = world_pos.x;
            y1 = world_pos.y;
        } else {
            x0 = world_pos.x;
            y0 = world_pos.y;
            x1 = flashlight_pos.x;
            y1 = flashlight_pos.y; 
        }
        assert(x0 <= x1);

        float slope = (y1 - y0) / (x1 - x0);

        float xleft = v->x;
        float xright = v->x + v->z;
        float ybottom = v->y;
        float ytop = v->y + v->w;
        assert(xleft <= xright);
        assert(ybottom <= ytop);

        float xcenter = (xleft + xright) / 2.0f;
        float ycenter = (ybottom + ytop) / 2.0f;

        {
            float left = xleft + 0.01f;
            float y = slope * (left - x0) + y0;
            if (ybottom < y && y < ytop && x0 < left && left < x1) {
                return true;
            }
        }

        {
            float right = xright - 0.01f;
            float y = slope * (right - x0) + y0;
            if (ybottom < y && y < ytop && x0 < right && right < x1) {
                return true;
            }
        }
        {
            float top = ytop - 0.01f;
            float x = (top - y0) / slope + x0;
            if (xleft < x && x < xright && min(y0, y1) < top && top < max(y0, y1)) {
                return true;
            }
        }
        {
            float bottom = ybottom + 0.01f;
            float x = (bottom - y0) / slope + x0;
            if (xleft < x && x < xright && min(y0, y1) < bottom && bottom < max(y0, y1)) {
                return true;
            }
        }
    }
    return false;
}

vec3s sum_pointlights_at_pixel(const struct light_array* pointlights, vec3s pixel_pos) {
    vec3s result = { 0.0f, 0.0f, 0.0f };

    for (int i = 0; i < pointlights->count; i++) {
        struct light *light = &pointlights->values[i];
        float d_squared = (pixel_pos.x - light->pos.x) * (pixel_pos.x - light->pos.x) +
                          (pixel_pos.y - light->pos.y) * (pixel_pos.y - light->pos.y) +
                          (pixel_pos.z - light->pos.z) * (pixel_pos.z - light->pos.z);
        float d = sqrt(d_squared);

        float attenuation = 0.0f; 
        switch (light->diameter) {
        case LD_0:
            break;
        case LD_1:
            attenuation = d > 0.5f ? 0.0f : min(1.0f / d - 2.0f, 1.0f); //covers 1 unit
            break;
        case LD_3:
            attenuation = d > 1.5f ? 0.0f : min(1.0f / d - 0.666f, 1.0f); //covers 3 units
            break;
        case LD_5:
            attenuation = d > 2.5f ? 0.0f : min(1.0f / d - 0.4f, 1.0f); //covers 5 units
            break;
        case LD_7:
            attenuation = d > 3.5f ? 0.0f : min(1.0f / d - 0.2857f, 1.0f); //covers 7 units
            break;
        case LD_9:
            attenuation = d > 4.5f ? 0.0f : min(1.0f / d - 0.2222f, 1.0f); //covers 9 units
            break;
        case LD_11:
            attenuation = d > 5.5f ? 0.0f : min(1.0f / d - 0.1818f, 1.0f); //covers 11 units
            break;
        default:
            assert(false);
        }

        //diffuse lighting
        vec3s diffuse = glms_vec3_scale(light->color, attenuation);
        vec3s sums = glms_vec3_scale(diffuse, 1.0f);
        result = glms_vec3_add(result, sums);
    }

    return result;
}

void renderer_rasterize(struct renderer *r/*, 
                        vec3s flashlight_pos, 
                        vec3s flashlight_dir, 
                        const struct vec4_array *walls,
                        const struct light_array *pointlights,
                        vec3s world_spotlight_pos*/) {
    assert(r->clips.count % 3 == 0);


    for (int i = 0; i < r->clips.count; i += 3) {
        float x0 =  r->clips.values[i].pos.x;
        float y0 =  r->clips.values[i].pos.y;
        float z0 =  r->clips.values[i].pos.z;
        float inv_w0 = r->clips.values[i].inv_w;
        float x1 =  r->clips.values[i + 1].pos.x;
        float y1 =  r->clips.values[i + 1].pos.y;
        float z1 =  r->clips.values[i + 1].pos.z;
        float inv_w1 = r->clips.values[i + 1].inv_w;
        float x2 =  r->clips.values[i + 2].pos.x;
        float y2 =  r->clips.values[i + 2].pos.y;
        float z2 =  r->clips.values[i + 2].pos.z;
        float inv_w2 = r->clips.values[i + 2].inv_w;

        float u0 = r->clips.values[i + 0].tex.u;
        float u1 = r->clips.values[i + 1].tex.u;
        float u2 = r->clips.values[i + 2].tex.u;
        float v0 = r->clips.values[i + 0].tex.v;
        float v1 = r->clips.values[i + 1].tex.v;
        float v2 = r->clips.values[i + 2].tex.v;
        struct texture *texture = r->clips.values[i].texture; //Note should be the same for all 3 positions


        //orientation test (ccw if area > 0)
        float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
       
        //cull backfaces
        //reversing sign of y-axis during view space -> clip space transformation
        //so area > 0.0f is back facing 
        if (area > 0.0f) {
            continue;
        }


        float xmin = min(min(x0, x1), x2);
        float xmax = max(max(x0, x1), x2);
        float ymin = min(min(y0, y1), y2);
        float ymax = max(max(y0, y1), y2);

        int xmin_i = (int)floorf(xmin);
        int xmax_i = (int)ceilf(xmax);
        int ymin_i = (int)floorf(ymin);
        int ymax_i = (int)ceilf(ymax);

        ymin_i = max(0, ymin_i);
        ymax_i = min(r->height - 1, ymax_i);
        xmin_i = max(0, xmin_i);
        xmax_i = min(r->width - 1, xmax_i);

        float A0 = y0 - y1;
        float B0 = x1 - x0;
        float C0 = x0 * y1 - x1 * y0;

        float A1 = y1 - y2;
        float B1 = x2 - x1;
        float C1 = x1 * y2 - x2 * y1;

        float A2 = y2 - y0;
        float B2 = x0 - x2;
        float C2 = x2 * y0 - x0 * y2;
        
        float E0_row = A0 * (xmin_i + 0.5f) + B0 * (ymin_i + 0.5f) + C0;
        float E1_row = A1 * (xmin_i + 0.5f) + B1 * (ymin_i + 0.5f) + C1;
        float E2_row = A2 * (xmin_i + 0.5f) + B2 * (ymin_i + 0.5f) + C2;


        //lighting for entire face computed once - flat lighting
        vec3s norm = glms_vec3_normalize(r->clips.values[i + 0].normal); //TODO assuming all normals point in same direction for now
        vec3s light_color = { 1.0f, 1.0f, 1.0f };

        vec3s to_pixel_dir = glms_vec3_normalize((vec3s) { 750.f, -100.0f, 500.f });
        //diffuse lighting
        float diff = max(glms_dot(norm, to_pixel_dir), 0.0f);
        vec3s diffuse = glms_vec3_scale(light_color, diff);

        //TODO: use some small 

        vec3s ambient = { 0.2f, 0.2f, 0.2f };
        vec3s sums = glms_vec3_add(ambient, diffuse);
        sums = (vec3s) { min(sums.x, 1.0f), min(sums.y, 1.0f), min(sums.z, 1.0f) }; //TODO: should scale down using max among three channels

        //FragColor = vec4(result, 1.0);
        /*
        red = (unsigned char) (result.x * 255.0f);
        green = (unsigned char) (result.y * 255.0f);
        blue = (unsigned char) (result.z * 255.0f);
        */
        //end light


        for (int y = ymin_i; y <= ymax_i; y++) {
            float E0 = E0_row;
            float E1 = E1_row;
            float E2 = E2_row;
            for (int x = xmin_i; x <= xmax_i; x++) {
                if (E0 <= 0.01f && E1 <= 0.01f && E2 <= 0.01f) {
                    int px_idx = y * r->width + x;
                    assert(px_idx < r->width * r->height);

                    float px = x + 0.5f;
                    float py = y + 0.5f;

                    float e0 = (x1 - px) * (y2 - py) - (y1 - py) * (x2 - px); // opposite v0
                    float e1 = (x2 - px) * (y0 - py) - (y2 - py) * (x0 - px); // opposite v1
                    float e2 = (x0 - px) * (y1 - py) - (y0 - py) * (x1 - px); // opposite v2

                    // Barycentrics
                    float b0 = e0 / area;
                    float b1 = e1 / area;
                    float b2 = e2 / area;

                    //perspective-correct depth TODO clean this up
                    float indepth = z0 * inv_w0 * b0 + z1 * inv_w1 * b1 + z2 * inv_w2 * b2;
                    float inv_w = inv_w0 * b0 + inv_w1 * b1 + inv_w2 * b2;
                    float depth = indepth / inv_w;

                    if (r->depths[px_idx] > depth) {
                        //perspective-correct textures  TODO clean this up
                        float u0_over_w = u0 * inv_w0;
                        float v0_over_w = v0 * inv_w0;
                        float u1_over_w = u1 * inv_w1;
                        float v1_over_w = v1 * inv_w1;
                        float u2_over_w = u2 * inv_w2;
                        float v2_over_w = v2 * inv_w2;

                        float u_over_w = u0_over_w * b0 + u1_over_w * b1 + u2_over_w * b2;
                        float v_over_w = v0_over_w * b0 + v1_over_w * b1 + v2_over_w * b2;
                        float u = u_over_w / inv_w;
                        float v = v_over_w / inv_w;
                    
                        int tx = min((int) (u * texture->width), texture->width - 1); //hardcoding texture sizes (32x32) for now
                        int ty = min((int) (v * texture->height), texture->height - 1);
                        tx = max(tx, 0);
                        ty = max(ty, 0);
                        int off = texture->channels * texture->width * ty + texture->channels * tx;

                        unsigned char red = texture->data[off];
                        unsigned char green = texture->data[off + 1];
                        unsigned char blue = texture->data[off + 2];

                        vec3s obj_color = { (float) red, (float) green, (float) blue };
                        vec3s result = glms_vec3_mul(sums, obj_color);
                        red = (unsigned char) (result.x);
                        green = (unsigned char) (result.y);
                        blue = (unsigned char) (result.z);


                        //TODO test lighting here
                        /*
                        {
                            vec3s world_p0 = r->clips.values[i + 0].world_pos; 
                            vec3s world_p1 = r->clips.values[i + 1].world_pos; 
                            vec3s world_p2 = r->clips.values[i + 2].world_pos; 
                            float world_x = world_p0.x * b0 + world_p1.x * b1 + world_p2.x * b2;
                            float world_y = world_p0.y * b0 + world_p1.y * b1 + world_p2.y * b2;
                            float world_z = world_p0.z * b0 + world_p1.z * b1 + world_p2.z * b2;
                            vec3s pixel_pos = { world_x, world_y, world_z };

                            vec3s to_pixel_dir = glms_vec3_normalize(glms_vec3_sub(flashlight_pos, pixel_pos));
                            float theta = glms_vec3_dot(to_pixel_dir, glms_vec3_scale(glms_vec3_normalize(flashlight_dir), -1.0f));
                            float outer_cutoff = cos(glm_rad(60));
                            float epsilon = cos(glm_rad(45.f)) - outer_cutoff;
                            float intensity = (theta - outer_cutoff) / epsilon;
                            intensity = glm_clamp(intensity, 0.0f, 1.0f);


                            float d_squared = (pixel_pos.x - flashlight_pos.x) * (pixel_pos.x - flashlight_pos.x) +
                                              (pixel_pos.y - flashlight_pos.y) * (pixel_pos.y - flashlight_pos.y) +
                                              (pixel_pos.z - flashlight_pos.z) * (pixel_pos.z - flashlight_pos.z);
                            float d = sqrt(d_squared);
                    

                            //flashlight attenuation
                            float attenuation = 1.0f / (1.0f + 0.045f * d + 0.0075f * d_squared);

                            vec3s norm = glms_vec3_normalize(r->clips.values[i + 0].normal); //TODO assuming all normals point in same direction for now
                            vec3s light_color = { 1.0f, 1.0f, 1.0f };

                            bool collided = line_collides_with_wall(flashlight_pos, pixel_pos, walls);


                            float ambient_mag = 0.0f; //TODO make this high if outside, low if inside
                            intensity = collided ? 0.0f : intensity;
                            vec3s ambient = glms_vec3_scale(light_color, ambient_mag * attenuation);

                            //world point lights
                            vec3s test_result = sum_pointlights_at_pixel(pointlights, pixel_pos);


                            //diffuse lighting
                            float diff = max(glms_dot(norm, to_pixel_dir), 0.0f) * intensity; //TODO change to 1.0 for a point light
                            vec3s diffuse = glms_vec3_scale(light_color, diff * attenuation);

                            vec3s sums = glms_vec3_add(ambient, diffuse);
                            sums = glms_vec3_add(sums, test_result);
                            sums = (vec3s) { min(sums.x, 1.0f), min(sums.y, 1.0f), min(sums.z, 1.0f) }; //TODO: should scale down using max among three channels

                            vec3s obj_color = { (float) red/255.0f, (float) green/255.0f, (float) blue/255.0f };
                            vec3s result = glms_vec3_mul(sums, obj_color);

                            //FragColor = vec4(result, 1.0);
                            red = (unsigned char) (result.x * 255.0f);
                            green = (unsigned char) (result.y * 255.0f);
                            blue = (unsigned char) (result.z * 255.0f);
                            //end light
                        }
                        */

                         
                        r->pixels[px_idx] = 0xFF000000 | (red << 16) | (green << 8) | (blue << 0);
                        r->depths[px_idx] = depth;
                    }
                }

                E0 += A0;
                E1 += A1;
                E2 += A2;
            }
            E0_row += B0;
            E1_row += B1;
            E2_row += B2;
        }
    }
}




#endif //RENDERER_H
