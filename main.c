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

#define WIDTH  (320 * 2)
#define HEIGHT (240 * 2)


static uint32_t *pixels;  // RGBA or BGRA depending on backend
static float t = 0.0f;

static void render(uint32_t *pixels) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int i = y * WIDTH + x;
            uint8_t r = (uint8_t) min(x, 255);
            uint8_t g = (uint8_t) min(y, 255);
            uint8_t b = ((sinf(t) + 1.0f) / 2.0f) * 255.f;
#ifdef _WIN32
            pixels[i] = (0xFFu << 24) | (r << 16) | (g << 8) | b; // ARGB
#else
            pixels[i] = (0xFFu << 24) | (b << 16) | (g << 8) | r; // ARGB
#endif 
        }
    }
}

struct app {
    struct camera camera;
    struct renderer renderer;
    unsigned char *background;
    struct gkab_arena arena;
    struct model model;
    int model_idx;
};

struct app g_app;

void app_init(struct app *app, uint32_t *pixels, int width, int height, const char *background_file, const char *model_file, const char *texture_file) {
    camera_init(&app->camera, (float) width / (float) height);
    camera_view(&app->camera, (vec3s) { 0.0f, 0.0f, 12.0f }, (vec3s) { 0.0f, 0.0f, 0.0f });

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
    gkab_arena_init(&app->arena);

    app->model_idx = 0;

    //model_load(&app->model, model_file, texture_file, glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f },  &app->arena);
    //model_load(&app->model, model_file, texture_file, glm_rad(0.0f), true, (vec3s) { 0.3f, 0.3f, 0.3f },  &app->arena);
    model_load(&g_app.model, "teapot.obj", "grey.png", glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
    printf("vertices: %d\n", app->model.positions.count);
}

#ifdef _WIN32
#include <windows.h>
#include "window.h"


int main(int argc, char **argv) {
    struct window window;
    window_init(&window, WIDTH, HEIGHT, WIDTH, HEIGHT);

    bool quit = false;
    double target = 1.0 / 60.0;
    double last = window_get_time();

    app_init(&g_app, window.pixels, window.surface_width, window.surface_height,
             "background.png", "cessna.obj", "grey.png");
             //"background.png", "minicooper.obj", "grey.png");
             //"background.png", "teapot.obj", "grey.png");

    double frame_times = 0.0;
   
    float rads = 0.0f;

    while (!quit) {
        double now = window_get_time();
        double dt = now - last;
        last = now;

        window_update_events(&quit);

        //user inputs
        bool changed_model = false;
        if (keyboard_key('W').down) {
            g_app.model_idx = min(g_app.model_idx + 1, 2);
            changed_model = true;
        }
        if (keyboard_key('S').down) {
            g_app.model_idx = max(g_app.model_idx - 1, 0);
            changed_model = true;
        }
        if (keyboard_key('A').is_down) {
            rads -= 0.1f;
        }
        if (keyboard_key('D').is_down) {
            rads += 0.1f;
        }

        if (changed_model) {
            gkab_arena_reset(&g_app.arena);
            switch (g_app.model_idx) {
            case 0:
                model_load(&g_app.model, "teapot.obj", "grey.png", glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
                break;
            case 1:
                model_load(&g_app.model, "cessna.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.3f, 0.3f, 0.3f },  &g_app.arena);
                break;
            case 2:
                model_load(&g_app.model, "minicooper.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
                break;
            default:
                assert(false);
                break;
            }
        }


        renderer_begin_frame(&g_app.renderer, g_app.background);


        mat4s identity = glms_mat4_identity();
        mat4s scale = identity;
        mat4s rotate = glms_rotate(identity, rads, (vec3s) { 1.0f, 1.0f, 0.0f });
        mat4s translate = glms_translate(identity, (vec3s) { 0.0f, 0.0f, 0.0f });
        mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

        submit_dynamic_mesh(&g_app.renderer, 
                            &g_app.model.positions,
                            &g_app.model.texcoords,
                            &g_app.model.normals,
                            &g_app.model.indices,
                            &g_app.model.texture,
                            g_app.camera.proj_view, 
                            model_mat, rotate);

        ui_draw_text(&g_app.renderer, "CONTROLS", (struct cell) { 0, 0 });
        renderer_rasterize(&g_app.renderer);

        window_blit(&window);

        double after = window_get_time();
        double frameTime = after - now;
        double remaining = target - frameTime;

        if (remaining > 0) {
            window_sleep(remaining * 1000.0);
        }
    }

    window_free(&window);
}

#endif // _WIN32

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

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

void increment_model_idx(void) {
    g_app.model_idx = min(g_app.model_idx + 1, 2);
    gkab_arena_reset(&g_app.arena);
    switch (g_app.model_idx) {
    case 0:
        model_load(&g_app.model, "teapot.obj", "grey.png", glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
        break;
    case 1:
        model_load(&g_app.model, "cessna.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.3f, 0.3f, 0.3f },  &g_app.arena);
        break;
    case 2:
        model_load(&g_app.model, "minicooper.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
        break;
    default:
        assert(false);
        break;
    }
}

void decrement_model_idx(void) {
    g_app.model_idx = max(g_app.model_idx - 1, 0);
    gkab_arena_reset(&g_app.arena);
    switch (g_app.model_idx) {
    case 0:
        model_load(&g_app.model, "teapot.obj", "grey.png", glm_rad(0.0f), false, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
        break;
    case 1:
        model_load(&g_app.model, "cessna.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.3f, 0.3f, 0.3f },  &g_app.arena);
        break;
    case 2:
        model_load(&g_app.model, "minicooper.obj", "grey.png", glm_rad(0.0f), true, (vec3s) { 0.05f, 0.05f, 0.05f },  &g_app.arena);
        break;
    default:
        assert(false);
        break;
    }
}

static void wasm_frame(void) {
    if (g_left) {
        g_rads -= 0.1f;
    }
    if (g_right) {
        g_rads += 0.1f;
    }


    renderer_begin_frame(&g_app.renderer, g_app.background);

    mat4s identity = glms_mat4_identity();
    mat4s scale = identity;
    mat4s rotate = glms_rotate(identity, g_rads, (vec3s) { 1.0f, 1.0f, 0.0f });
    mat4s translate = glms_translate(identity, (vec3s) { 0.0f, 0.0f, 0.0f });
    mat4s model_mat = glms_mat4_mul(glms_mat4_mul(translate, rotate), scale);

    submit_dynamic_mesh(&g_app.renderer, 
                        &g_app.model.positions,
                        &g_app.model.texcoords,
                        &g_app.model.normals,
                        &g_app.model.indices,
                        &g_app.model.texture,
                        g_app.camera.proj_view, 
                        model_mat, rotate);

    renderer_rasterize(&g_app.renderer);
    ui_draw_text(&g_app.renderer, "CONTROLS", (struct cell) { 0, 0 });
    js_draw_pixels(pixels, WIDTH, HEIGHT);
}

int main(void) {
    pixels = (uint32_t *)malloc(WIDTH * HEIGHT * 4);
    app_init(&g_app, pixels, WIDTH, HEIGHT,
             "/background.png", "/cessna.obj", "/grey.png");
    g_left = false;
    g_right = false;

    EM_ASM({
        const set_left = Module._set_left;
        const set_right = Module._set_right;
        const decrement_model_idx = Module._decrement_model_idx;
        const increment_model_idx = Module._increment_model_idx;

        document.addEventListener("keydown", e => {
            if (e.key === "Escape") console.log("pressed escape");
        });
        document.addEventListener("keydown", e => {
            if (e.key === "ArrowLeft") set_left(true);
        });
        document.addEventListener("keyup", e => {
            if (e.key === "ArrowLeft") set_left(false);
        });
        document.addEventListener("keydown", e => {
            if (e.key === "ArrowRight") set_right(true);
        });
        document.addEventListener("keyup", e => {
            if (e.key === "ArrowRight") set_right(false);
        });
        document.addEventListener("keydown", e => {
            if (e.key === "ArrowUp") increment_model_idx();
        });
        document.addEventListener("keydown", e => {
            if (e.key === "ArrowDown") decrement_model_idx();
        });
    });

    emscripten_set_main_loop(wasm_frame, 0, 1);
    return 0;
}

#endif // __EMSCRIPTEN__

