#ifndef MODEL_H
#define MODEL_H

#include "array.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum light_diameter {
    LD_0 = 0, //off
    LD_1,
    LD_3, //can be next to wall without bleeding into next room
    LD_5,
    LD_7,
    LD_9,
    LD_11
};

struct light {
    vec3s pos;
    enum light_diameter diameter;
    vec3s color; 
};

GKAB_MAKE_ARRAY(light_array, struct light);

struct texture {
    unsigned char *data;
    int width;
    int height;
    int channels;
};

struct model {
    struct vec3_array positions;
    struct vec2_array texcoords;
    struct vec3_array normals;
    struct indices indices;

    struct texture texture;
};

void mesh_load(struct model *m, const char *path, float rot, bool swap_winding, vec3s scale, struct gkab_arena *arena) {
    FILE *f = fopen(path, "r");
    if (!f) {
        exit(1);
    }

    vec3_array_init(&m->positions, arena);
    vec3_array_init(&m->normals, arena);
    vec2_array_init(&m->texcoords, arena);
    iarray_init(&m->indices.positions, arena);
    iarray_init(&m->indices.texcoords, arena);
    iarray_init(&m->indices.normals, arena);
    
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        char c = *buf;
        switch (c) {
        case '\n':
        case '#':
            break;
        case 'v': {
            if (*(buf + 1) == 't') {
                char *p = buf + 2;
                float x = strtof(p, &p);
                float y = strtof(p, &p);
                vec2_array_append(&m->texcoords, (vec2s) { x, y });
                //TODO handle possible 3rd texture coordinate (w)
                break;
            }
            if (*(buf + 1) == 'n') {
                char *p = buf + 2;
                float x = strtof(p, &p);
                float y = strtof(p, &p);
                float z = strtof(p, &p);
                vec3_array_append(&m->normals, (vec3s) { x, y, z });
                break;
            }
            char *p = buf + 1;
            float x = strtof(p, &p);
            float y = strtof(p, &p);
            float z = strtof(p, &p);
            //TODO handle possible 4th position (w)
            vec3_array_append(&m->positions, (vec3s) { x, y, z });
            break;
        }
        case 'f': {
            //      f 1 2 3             //positions only
            //      f 3/2 5/3 5/2       //positions and texture
            //      f 1//2 3//5 2//2    //positions and normal
            //      f 1/2/2 3/3/5 2/2/2 //positions, textures and normal

            char *p = buf + 1;
            int count = 0;
            while (*p != '\n') {
            //for (int i = 0; i < 3; i++) {
                int pos_idx = 0;
                int texcoord_idx = 0;
                int normal_idx = 0;

                pos_idx = strtol(p, &p, 10);
                /*
                if (*p == '\n') {
                    printf("count; %d\n", count);
                    break;
                }
                count++;
                */

                if (*p == '/' && *(p + 1) == '/') { //position and normal
                    p += 2;
                    normal_idx = strtol(p, &p, 10);
                } else if (*p == '/') { //position, texture and possibly normal
                    p++;
                    texcoord_idx = strtol(p, &p, 10); 
                    if (*p == '/') {
                        p++;
                        normal_idx = strtol(p, &p, 10);
                    }
                }

                pos_idx = pos_idx < 0 ? m->positions.count + pos_idx : pos_idx - 1;
                texcoord_idx = texcoord_idx < 0 ? m->texcoords.count + texcoord_idx : texcoord_idx - 1;
                normal_idx = normal_idx < 0 ? m->normals.count + normal_idx : normal_idx - 1;


                if (pos_idx != -1) {
                    //faces may be polygon with more than 3 vertices
                    //in that case, split into triangles by appending first/prev index
                    if (count > 2) {
                        int prev_idx = m->indices.positions.values[m->indices.positions.count - 1];
                        int first_idx = m->indices.positions.values[m->indices.positions.count - count];
                        iarray_append(&m->indices.positions, first_idx);
                        iarray_append(&m->indices.positions, prev_idx);
                        count += 2;
                    }
                    iarray_append(&m->indices.positions, pos_idx);
                }

                if (texcoord_idx != -1) {
                    iarray_append(&m->indices.texcoords, texcoord_idx);
                }

                if (normal_idx != -1) {
                    iarray_append(&m->indices.normals, normal_idx);
                }

                count++;
            }
            
            break;
        }
        default:
            printf(".obj identifier '%c' not supported.\n", c);
            break;
        }
    }

    fclose(f);

    vec3s axis = { 0.0f, 0.0f, 1.0f };
    for (int i = 0; i < m->positions.count; i++) {
        vec3s pos = m->positions.values[i];
        m->positions.values[i] = glms_vec3_rotate(glms_vec3_mul(pos, scale), rot, axis); 
    }

    if (swap_winding) {
        for (int i = 0; i < m->indices.positions.count; i += 3) {
            int t = m->indices.positions.values[i]; 
            m->indices.positions.values[i] = m->indices.positions.values[i + 1];
            m->indices.positions.values[i + 1] = t;
        }
    }

    printf("%d, %d\n", m->indices.positions.count, m->indices.normals.count);
    if (m->indices.normals.count == 0) {
        m->normals.count = 0;
        for (int i = 0; i < m->indices.positions.count; i += 3) {
            vec3s p0 = m->positions.values[m->indices.positions.values[i + 0]];
            vec3s p1 = m->positions.values[m->indices.positions.values[i + 1]];
            vec3s p2 = m->positions.values[m->indices.positions.values[i + 2]];

            vec3s e1 = glms_vec3_sub(p1, p0);
            vec3s e2 = glms_vec3_sub(p2, p0);
            vec3s normal = glms_vec3_normalize(glms_vec3_cross(e2, e1));

            int idx = m->normals.count;
            iarray_append(&m->indices.normals, idx);
            iarray_append(&m->indices.normals, idx);
            iarray_append(&m->indices.normals, idx);

            vec3_array_append(&m->normals, normal);
        }
    }

    printf("%d, %d\n", m->indices.positions.count, m->indices.normals.count);
    assert(m->indices.positions.count == m->indices.normals.count);
}

void texture_load(struct texture *t, const char *path) {
    stbi_set_flip_vertically_on_load(false);
    t->data = stbi_load(path, &t->width, &t->height, &t->channels, 0);
    printf("channels: %d\n", t->channels);
    if (!t->data) {
        printf("failed to load '%s'\n", path);
        exit(1);
    }
}

void model_load(struct model *m, const char *mesh_path, const char *texture_path, float rot, bool swap_winding, vec3s scale, struct gkab_arena *arena) {
    mesh_load(m, mesh_path, rot, swap_winding, scale, arena);
    texture_load(&m->texture, texture_path);
}

void model_free(struct model *m) {
    stbi_image_free(m->texture.data);
}

#endif //MODEL_H
