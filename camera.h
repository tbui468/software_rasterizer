#ifndef CAMERA_H
#define CAMERA_H

#include "cglm/struct.h"

struct camera {
    mat4s proj;
    vec3s up;
    mat4s proj_view;
};

void camera_init(struct camera *c, float aspect_ratio) {
    float fov = glm_rad(45.0f);
    float near_plane = 0.1f;
    float far_plane = 100.0f;

    c->proj = glms_perspective(fov, aspect_ratio, near_plane, far_plane);
    c->up = glms_normalize((vec3s){ 0.0f, 1.0f, 0.0f });
}

void camera_view(struct camera *c, vec3s from, vec3s to) {
    mat4s view = glms_lookat(from, to, c->up);
    c->proj_view = glms_mat4_mul(c->proj, view);
}

#endif 
