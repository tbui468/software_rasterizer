#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#include "renderer.h"
#include "model.h"
#include "ui.h"
#include "camera.h"
#include "cglm/struct.h"

#define WIDTH  (1280 * 1)
#define HEIGHT (640 * 1)

enum scene_type {
    ST_TEAPOT = 0,
    ST_CESSNA,
    ST_MINICOOPER,
    ST_TEXTURE,
    ST_MARS,
    ST_END
};

struct model_scene {
    struct model model;
};

struct texture_scene {
    struct model models[8];
};

struct mars_scene {
    struct model model;
    int16_t *elevation;
    int width, height;
    unsigned char *colormap;
};

struct scene {
    enum scene_type type;

    union {
        struct model_scene model;
        struct texture_scene texture;
        struct mars_scene mars;
    } as;

    struct camera camera;
};

struct app {
    struct renderer renderer;
    struct ui ui;
    struct gkab_arena arena;

    unsigned char *background;
    struct scene scene;
};

void scene_init(struct scene *s, enum scene_type type, struct gkab_arena *arena) {
    s->type = type;

    switch (type) {
    case ST_TEAPOT:
        camera_view(&s->camera, (vec3s) { 0.0f, 0.0f, 12.0f }, (vec3s) { 0.0f, 0.0f, 0.0f });
        model_load(&s->as.model.model, "teapot.obj", "grey.png", glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f }, (vec3s) { 0.0f, 0.0f, 0.0f }, arena);
        break;
    case ST_CESSNA:
        camera_view(&s->camera, (vec3s) { 0.0f, 0.0f, 12.0f }, (vec3s) { 0.0f, 0.0f, 0.0f });
        model_load(&s->as.model.model, "cessna.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.3f, 0.3f, 0.3f }, (vec3s) { 0.0f, 0.0f, 0.0f },  arena);
        break;
    case ST_MINICOOPER: 
        camera_view(&s->camera, (vec3s) { 0.0f, 0.0f, 12.0f }, (vec3s) { 0.0f, 0.0f, 0.0f });
        model_load(&s->as.model.model, "minicooper.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.05f, 0.05f, 0.05f }, (vec3s) { 0.0f, 10.0f, -20.0f },  arena);
        break;
    case ST_TEXTURE:
        camera_view(&s->camera, (vec3s) { 0.0f, 0.0f, 12.0f }, (vec3s) { 0.0f, 0.0f, 0.0f });
        for (int i = 0; i < 1; i++) {
            model_load(&s->as.texture.models[i], "cube.obj", "color.png", glm_rad(0.0f), false, (vec3s) { 4.0f, 4.0f, 4.0f }, (vec3s) { 0.0f, 0.0f, 0.0f },  arena);
        }
        break;
    case ST_MARS: {
        printf("init mars\n");
        uint16_t width, height;
        {
            FILE *f = fopen("mars.bin", "rb");
            fread(&width, sizeof(uint16_t), 1, f);
            fread(&height, sizeof(uint16_t), 1, f);
            assert(width == WIDTH && height == HEIGHT);
            s->as.mars.width = width;
            s->as.mars.height = height;
            s->as.mars.elevation = malloc(width * height * sizeof(int16_t));
            fread(s->as.mars.elevation, sizeof(int16_t), width * height, f);
            fclose(f);
        }

        int16_t mn = s->as.mars.elevation[0];
        int16_t mx = s->as.mars.elevation[0];
#define BUCKET_COUNT 32
        int counts[BUCKET_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0 };
#define BUCKET_WIDTH (65536 / BUCKET_COUNT )

        for (int i = 1; i < width * height; i++) {
            if (s->as.mars.elevation[i] < mn) mn = s->as.mars.elevation[i];
            if (s->as.mars.elevation[i] > mx) mx = s->as.mars.elevation[i];
            int32_t v = (int32_t) (s->as.mars.elevation[i]) + 32768;
            counts[v / BUCKET_WIDTH]++;
        }

        {
            int bg_width, bg_height, bg_channels;
            stbi_set_flip_vertically_on_load(true);
            s->as.mars.colormap = stbi_load("cm2_orange.png", &bg_width, &bg_height, &bg_channels, 0);
            if (!s->as.mars.colormap) {
                printf("failed to load background image\n");
                exit(1);
            }

            for (int i = 0; i < bg_width * bg_height; i++) {
                unsigned char *pixel = s->as.mars.colormap + i * 4;
                unsigned char r = pixel[0];
                unsigned char g = pixel[1];
                unsigned char b = pixel[2];

#ifdef _WIN32
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
#endif
#ifdef __EMSCRIPTEN__
                pixel[2] = b;
                pixel[1] = g;
                pixel[0] = r;
#endif
            }
        }


        float ratio = (float) width / height;

        struct texture texture;
        texture.data = malloc(width * height * sizeof(unsigned char) * 4);
        texture.width = width;
        texture.height = height;
        texture.channels = 4;
        struct gkab_arena arena;
        gkab_arena_init(&arena);

        model_unit_sphere(&s->as.mars.model, 0.0f, (vec3s) { 4.0f, 4.0f, 4.0f }, (vec3s) { 0.0f, 0.0f, 0.0f }, &arena);
        s->as.mars.model.texture = texture;

        float rad = M_PI / 4.0f + M_PI;
        const float step = M_PI / 6.0f;
        float sin_a[4] = { sinf(rad), sinf(rad + step), sinf(rad + step * 2.0f), sinf(rad - step) };
        float cos_a[4] = { cosf(rad), cosf(rad + step), cosf(rad + step * 2.0f), cosf(rad - step) };

        const float light_height = M_PI / 3.0f;

        for (int i = 0; i < width * height; i++) {
            int32_t v = (int32_t) (s->as.mars.elevation[i]) + 7626;
            int offset = (v / 113 / 2) * 4;
            //only using subset of 128 buckets
            //offset = (offset / 16) * 16 + 8;

            int row = i / width;
            int col = i % width;

            uint8_t red =   s->as.mars.colormap[offset + 2];
            uint8_t green = s->as.mars.colormap[offset + 1];
            uint8_t blue =  s->as.mars.colormap[offset + 0];

            float intensity = 0.0f;
            if (row > 0 && row < height - 1 && col > 0 && col < width - 1 && s->as.mars.elevation[i] > -2000) {
                int32_t right_offset = ((int32_t) (s->as.mars.elevation[row * width + col + 1]) + 7626) / 113 / 2 * 4;
                right_offset = right_offset / 16 * 16 + 8;
                int32_t left_offset = ((int32_t) (s->as.mars.elevation[row * width + col - 1]) + 7626) / 113 / 2 * 4;
                left_offset = left_offset / 16 * 16 + 8;
                float dz_dx = (right_offset - left_offset) / 2.0f;
                int32_t bottom_offset = ((int32_t) (s->as.mars.elevation[(row + 1) * width + col]) + 7626) / 113 / 2 * 4;
                bottom_offset = bottom_offset / 16 * 16 + 8;
                int32_t top_offset = ((int32_t) (s->as.mars.elevation[(row - 1) * width + col]) + 7626) / 113 / 2 * 4;
                top_offset = top_offset / 16 * 16 + 8;
                float dz_dy = (bottom_offset - top_offset) / 2.0f;
                
                dz_dy /= 20.0f;
                dz_dx /= 20.0f;


                //float slope = dz_dx * s + dz_dy * c;
            
                float denom = sqrtf(1.0f + dz_dx * dz_dx + dz_dy * dz_dy);
                float nx = -dz_dx / denom;
                float ny = -dz_dy / denom;
                float nz = 1.0f / denom;

                for (int i = 0; i < 4; i++) {
                    float lx = cosf(light_height) * sin_a[i];
                    float ly = cosf(light_height) * cos_a[i];
                    float lz = sinf(light_height);
                    intensity += max(0.0f, nx * lx + ny * ly + nz * lz); 
                }
                intensity /= 4.0f;
                if (s->as.mars.elevation[i] > 3000 && s->as.mars.elevation[i] <= 4000)
                    intensity = pow(intensity, 0.2f); 
                else if (s->as.mars.elevation[i] > 4000 && s->as.mars.elevation[i] <= 5000)
                    intensity = pow(intensity, 0.1f); 
                else if (s->as.mars.elevation[i] > 5000 && s->as.mars.elevation[i] <= 6000)
                    intensity = pow(intensity, 0.05f); 
                else if (s->as.mars.elevation[i] > 6000 && s->as.mars.elevation[i] <= 7000)
                    intensity = pow(intensity, 0.1f); 
                else if (s->as.mars.elevation[i] > 7000 && s->as.mars.elevation[i] <= 8000)
                    intensity = pow(intensity, 0.2f); 
                else
                    intensity = pow(intensity, 0.3f); 
            } else {
                intensity = 1.0f;
            }

            red = (uint8_t) (((float) red / 255.f) * intensity * 255);
            green = (uint8_t) (((float) green / 255.f) * intensity * 255);
            blue = (uint8_t) (((float) blue / 255.f) * intensity * 255);

            s->as.mars.model.texture.data[i * 4 + 2] = blue;
            s->as.mars.model.texture.data[i * 4 + 1] = green;
            s->as.mars.model.texture.data[i * 4 + 0] = red;
            s->as.mars.model.texture.data[i * 4 + 3] = 0xFF;
        }
        break;
    }
    default:
        assert(false);
        break;

    }
}

void scene_update(struct scene *s) {

}

void scene_draw(struct scene *s, struct renderer *r, float rads) {
    switch (s->type) {
    case ST_TEAPOT:
    case ST_CESSNA:
    case ST_MINICOOPER: {
        mat4s identity = glms_mat4_identity();
        mat4s scale = identity;
        mat4s rotate = glms_rotate(identity, rads, (vec3s) { 1.0f, 1.0f, 0.0f });
        mat4s translate = glms_translate(identity, (vec3s) { 0.0f, 0.0f, 0.0f });
        mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

        submit_dynamic_mesh(r, 
                            &s->as.model.model.positions,
                            &s->as.model.model.texcoords,
                            &s->as.model.model.normals,
                            &s->as.model.model.indices,
                            &s->as.model.model.texture,
                            s->camera.proj_view, 
                            model_mat, rotate);
        break;
    }
    case ST_TEXTURE: {
        /*
        float xs[4] = { -2.0f, 2.0f, -2.0f, 2.0f };
        float ys[4] = { 2.0f, 2.0f, -2.0f, -2.0f };
        float zs[8] = { 0.0f, 0.0f, 0.0f, 0.0f, -16.0f, -16.0f, -16.0f, -16.0f };
        
        
        for (int i = 0; i < 8; i++) {
            mat4s identity = glms_mat4_identity();
            mat4s scale = identity;
            mat4s rotate = glms_rotate(identity, rads, (vec3s) { 1.0f, 1.0f, 0.0f });
            mat4s translate = glms_translate(identity, (vec3s) { xs[i % 4], ys[i % 4], zs[i] });
            mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

            submit_dynamic_mesh(r, 
                                &s->as.model.model.positions,
                                &s->as.model.model.texcoords,
                                &s->as.model.model.normals,
                                &s->as.model.model.indices,
                                &s->as.model.model.texture,
                                s->camera.proj_view, 
                                model_mat, rotate);
        }
        */
            mat4s identity = glms_mat4_identity();
            mat4s scale = identity;
            mat4s rotate = glms_rotate(identity, rads, (vec3s) { 1.0f, 1.0f, 0.0f });
            mat4s translate = glms_translate(identity, (vec3s) { 0.0f, 0.0f, 0.0f });
            mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

            submit_dynamic_mesh(r, 
                                &s->as.model.model.positions,
                                &s->as.model.model.texcoords,
                                &s->as.model.model.normals,
                                &s->as.model.model.indices,
                                &s->as.model.model.texture,
                                s->camera.proj_view, 
                                model_mat, rotate);
        break;
    }
    case ST_MARS: {

        mat4s identity = glms_mat4_identity();
        mat4s scale = identity;
        mat4s rotate = glms_rotate(identity, rads, (vec3s) { 1.0f, 1.0f, 0.0f });
        mat4s translate = glms_translate(identity, (vec3s) { 0.0f, 0.0f, 0.0f });
        mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

        submit_dynamic_mesh(r, 
                            &s->as.mars.model.positions,
                            &s->as.mars.model.texcoords,
                            &s->as.mars.model.normals,
                            &s->as.mars.model.indices,
                            &s->as.mars.model.texture,
                            s->camera.proj_view, 
                            model_mat, rotate);

        break;
    }
    default:
        assert(false);
        break;
    }
}

struct app g_app;

void app_init(struct app *app, uint32_t *pixels, int width, int height, const char *background_file) {
    {
        int bg_width, bg_height, bg_channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *bg_data = stbi_load(background_file, &bg_width, &bg_height, &bg_channels, 0);
        printf("channels %d\n", bg_channels);
        printf("width %d\n", bg_width);
        printf("height %d\n", bg_height);
        if (!bg_data) {
            printf("failed to load background image\n");
            exit(1);
        }

        for (int i = 0; i < bg_width * bg_height; i++) {
            unsigned char *pixel = bg_data + i * 4;
            unsigned char r = pixel[0];
            unsigned char g = pixel[1];
            unsigned char b = pixel[2];
            pixel[0] = b;
            pixel[1] = g;
            pixel[2] = r;
        }

        app->background = bg_data;
    }

    init_char_map();

    renderer_init(&app->renderer, pixels, width, height);
    ui_init(&app->ui, &app->renderer);
    gkab_arena_init(&app->arena);

    camera_init(&app->scene.camera, (float) width / (float) height);

    scene_init(&app->scene, ST_TEAPOT, &app->arena);
}

#ifdef _WIN32
#include <windows.h>
#include "window.h"

int main(int argc, char **argv) {
    struct window window;
    window_init(&window, WIDTH, HEIGHT, WIDTH, HEIGHT, false);

    bool quit = false;
    double target = 1.0 / 60.0;
    double last = window_get_time();

    app_init(&g_app, window.pixels, window.surface_width, window.surface_height, "background.png");

    double frame_times = 0.0;

    float fps[60];
    int fps_idx = 0;
   
    float rads = 0.0f;
    enum scene_type scene_type = ST_TEAPOT;

    while (!quit) {
        double now = window_get_time();
        double dt = now - last;
        last = now;

        window_update_events(&quit);

        //user inputs
        if (keyboard_key('W').down) {
            scene_type = min(scene_type + 1, ST_END - 1);
            gkab_arena_reset(&g_app.arena);
            scene_init(&g_app.scene, scene_type, &g_app.arena);
        }
        if (keyboard_key('S').down) {
            scene_type = max(scene_type - 1, ST_TEAPOT);
            gkab_arena_reset(&g_app.arena);
            scene_init(&g_app.scene, scene_type, &g_app.arena);
        }
        if (keyboard_key('A').is_down) {
            rads -= 3.0f * dt;
        }
        if (keyboard_key('D').is_down) {
            rads += 3.0f * dt;
        }

        scene_update(&g_app.scene);

        renderer_begin_frame(&g_app.renderer, g_app.background);

        scene_draw(&g_app.scene, &g_app.renderer, rads);

        renderer_rasterize(&g_app.renderer);

        ui_begin(&g_app.ui);
        ui_set_color(&g_app.ui, CL_BLACK);

        fps[fps_idx] = 1.0 / dt / 60.0f;
        fps_idx = (fps_idx + 1) % 60;
        float avg_fps = 0.0f;
        for (int i = 0; i < 60; i++) {
            avg_fps += fps[i];
        }

        char buf[32];
        sprintf(buf, "FPS: %d", (int) avg_fps);
        ui_draw_text(&g_app.ui, buf, (struct cell) { 16, 8 });
        ui_draw_text(&g_app.ui, "LEFT / RIGHT ARROWS: ROTATE MODEL", (struct cell) { 32, 8 });
        ui_draw_text(&g_app.ui, "UP / DOWN ARROWS: CHANGE SCENE", (struct cell) { 48, 8 });

        window_blit(&window);

        double after = window_get_time();
        double frameTime = after - now;
        double remaining = target - frameTime;

        /*
        if (remaining > 0) {
            window_sleep(remaining * 1000.0);
        }
        */
    }

    window_free(&window);
}

#endif // _WIN32

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static uint32_t *pixels;  // RGBA or BGRA depending on backend

EM_JS(void, js_draw_pixels, (uint32_t *ptr, int w, int h), {
    if (!Module.canvasCtx) {
        var canvas = document.getElementById('canvas');
        canvas.width  = w;
        canvas.height = h;
        Module.canvasCtx = canvas.getContext('2d');
        Module.imageData = Module.canvasCtx.createImageData(w, h);
    }

    var ctx = Module.canvasCtx;
    var img = Module.imageData;

    var src = new Uint8ClampedArray(Module.HEAPU8.buffer,
                                    ptr, w * h * 4);
    //img.data.set(src);
    //swap rows so match gdi
    for (let y = 0; y < h; y++) {
      img.data.set(
        src.subarray((h - 1 - y) * w * 4, (h - y) * w * 4),
        y * w * 4
      );
    }

    ctx.putImageData(img, 0, 0);
});


static float g_rads = 0.0f;
bool g_left;
bool g_right;
void set_left(bool v) {
    g_left = v;
}
void set_right(bool v) {
    g_right = v;
}

enum scene_type g_scene_type;
void next_scene(void) {
    g_scene_type = min(g_scene_type + 1, ST_END - 1);
    gkab_arena_reset(&g_app.arena);
    scene_init(&g_app.scene, g_scene_type, &g_app.arena);
}

void prev_scene(void) {
    g_scene_type = g_scene_type == ST_TEAPOT ? ST_TEAPOT : g_scene_type - 1;
    gkab_arena_reset(&g_app.arena);
    scene_init(&g_app.scene, g_scene_type, &g_app.arena);
}


static void wasm_frame(void) {
    if (g_left) {
        g_rads -= 0.1f;
    }
    if (g_right) {
        g_rads += 0.1f;
    }

    renderer_begin_frame(&g_app.renderer, g_app.background);

    scene_draw(&g_app.scene, &g_app.renderer, g_rads);


    renderer_rasterize(&g_app.renderer);

    ui_begin(&g_app.ui);

    ui_draw_text(&g_app.ui, "A / D: ROTATE MODEL", (struct cell) { 32, 8 });
    ui_draw_text(&g_app.ui, "W / S: CHANGE SCENE", (struct cell) { 48, 8 });

    js_draw_pixels(pixels, WIDTH, HEIGHT);
}

int main(void) {
    pixels = (uint32_t *)malloc(WIDTH * HEIGHT * 4);
    app_init(&g_app, pixels, WIDTH, HEIGHT, "/background.png"); // "/cessna.obj", "/grey.png");
    g_left = false;
    g_right = false;
    g_scene_type = ST_TEAPOT;

    EM_ASM({
        const set_left = Module._set_left;
        const set_right = Module._set_right;
        const next_scene = Module._next_scene;
        const prev_scene = Module._prev_scene;

        document.addEventListener("keydown", e => {
            if (e.key === "Escape") console.log("pressed escape");
        });
        document.addEventListener("keydown", e => {
            if (e.key === "a") set_left(true);
        });
        document.addEventListener("keyup", e => {
            if (e.key === "a") set_left(false);
        });
        document.addEventListener("keydown", e => {
            if (e.key === "d") set_right(true);
        });
        document.addEventListener("keyup", e => {
            if (e.key === "d") set_right(false);
        });
        document.addEventListener("keydown", e => {
            if (e.key === "w") next_scene();
        });
        document.addEventListener("keydown", e => {
            if (e.key === "s") prev_scene();
        });
    });

    emscripten_set_main_loop(wasm_frame, 0, 1);
    return 0;
}

#endif // __EMSCRIPTEN__

