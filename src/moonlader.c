#define BJ_AUTOMAIN_CALLBACKS
#include <banjo/assert.h>
#include <banjo/audio.h>
#include <banjo/bitmap.h>
#include <banjo/draw.h>
#include <banjo/event.h>
#include <banjo/math.h>
#include <banjo/linmath.h>
#include <banjo/log.h>
#include <banjo/main.h>
#include <banjo/shader.h>
#include <banjo/system.h>
#include <banjo/time.h>
#include <banjo/window.h>

#include <math.h>

#define SCREEN_W 800
#define SCREEN_H 600

#define CANVAS_W 800
#define CANVAS_H 600

#define LANDER_VERTICES_LEN 24
#define LANDER_EDGES_LEN 25

typedef struct {
    bj_window*   window;
    bj_bitmap*   drawbuffer;
    bj_bitmap*   framebuffer;
    bj_stopwatch stopwatch;

    struct { float radius; float angle;} lander_coords[LANDER_VERTICES_LEN];
    size_t lander_edges[LANDER_EDGES_LEN][2];
} game_data;

static void prepare_assets(game_data* data) {

    const float lander_coords[][2] = {
        {-3.f, -7.f,},  {-7.f, -3.f,}, {-7.f, 3.f,},   {-3.f, 7.f,},
        {3.f, 7.f,},    {7.f, 3.f,},   {7.f, -3.f,},   {3.f, -7.f,},
        {-8.f, 8.f,},   {-8.f, 13.f,}, {8.f, 13.f,},   {8.f, 8.f,},
        {-3.f, 8.f,},   {3.f, 8.f,},   {-4.f, 13.f,},  {-7.f, 16.f,},
        {7.f, 16.f,},   {4.f, 13.f,},  {-10.f, 13.f,}, {-13.f, 17.f,},
        {-10.f, 17.f,}, {10.f, 12.f,}, {10.f, 17.f,},  {13.f, 17.f,},
    };

    const size_t lander_edges[][2] = {
        // Head
        {0, 1,}, {1, 2,}, {2, 3,}, {3, 4,}, {4, 5,}, {5, 6,}, {6, 7,}, {7, 0,}, 
        // Base
        {8, 9,},   {9, 10,}, {10, 11,},  {11, 8,},  
        // Neck
        {3, 12,},  {4, 13,},
        // Left leg
        {8, 19,}, {19, 20,},  {9, 20,}, {9, 19,},  
        // Right leg
        {11, 23,}, {22, 23,}, {10, 22, }, {10, 23,}, 
        // Thrusters
        {14, 15,}, {15, 16,}, {16, 17,},
    };

    for(size_t c = 0 ; c < LANDER_VERTICES_LEN ; ++c) {
        const float x = lander_coords[c][0];
        const float y = lander_coords[c][1];
        data->lander_coords[c].radius = sqrt(x * x + y * y);
        data->lander_coords[c].angle  = atan2f(y, x);
    }

    bj_memcpy(data->lander_edges, lander_edges, sizeof(lander_edges));
}


void draw(bj_bitmap* target, const game_data* data, double dt) {
    // rotation state and speed (radians per second)
    static float theta = 0.0f;
    const float angular_speed = .3f; // tweak this to taste
    theta += (float)dt * angular_speed;

    // keep theta bounded to avoid float growth
    const float TWO_PI = 6.28318530718f;
    if (theta > TWO_PI || theta < -TWO_PI) theta = fmodf(theta, TWO_PI);

    const uint32_t color = bj_bitmap_pixel_value(target, 0x00, 0xFF, 0x00);
    bj_bitmap_clear(target);

    const int x = CANVAS_W / 2;
    const int y = CANVAS_H / 2;
    const float size_em = 16.f;

    for (size_t e = 0; e < LANDER_EDGES_LEN; ++e) {
        const float r0 = data->lander_coords[data->lander_edges[e][0]].radius;
        const float a0 = data->lander_coords[data->lander_edges[e][0]].angle + theta;
        const float r1 = data->lander_coords[data->lander_edges[e][1]].radius;
        const float a1 = data->lander_coords[data->lander_edges[e][1]].angle + theta;

        bj_bitmap_draw_line(
            target,
            x + bj_cosf(a0) * r0 * size_em, y + bj_sinf(a0) * r0 * size_em,
            x + bj_cosf(a1) * r1 * size_em, y + bj_sinf(a1) * r1 * size_em,
            color
        );
    }
}

void key_callback(bj_window* p_window, const bj_key_event* e) {

    if(e->key == BJ_KEY_ESCAPE && e->action == BJ_RELEASE) {
        bj_window_set_should_close(p_window);
    }
}

int bj_app_begin(void** user_data, int argc, char* argv[]) {
    (void)argc; (void)argv;

    bj_error* p_error = 0;
    if(!bj_begin(&p_error)) {
        bj_err("Error 0x%08X: %s", p_error->code, p_error->message);
        return bj_callback_exit_error;
    } 

    game_data* data  = bj_malloc(sizeof(game_data));
    data->window      = bj_window_new("Moonlander", 0,0, SCREEN_W, SCREEN_H, 0);
    data->framebuffer = bj_window_get_framebuffer(data->window, 0);
    data->drawbuffer  = bj_bitmap_new(CANVAS_W, CANVAS_H, bj_bitmap_mode(data->framebuffer), 0);
    *user_data = data;

    prepare_assets(data);


    /* bj_set_key_callback(key_callback); */

    bj_set_key_callback(bj_close_on_escape);


    return bj_callback_continue;
}

int bj_app_iterate(void* user_data) {
    game_data* data = (game_data*)user_data;

    bj_dispatch_events();
    /* game_logic(bj_stopwatch_step_delay(&stopwatch)); */


    draw(data->drawbuffer, data, bj_stopwatch_step_delay(&data->stopwatch));
    bj_bitmap_blit_stretched(data->drawbuffer, 0, data->framebuffer, 0, BJ_BLIT_OP_COPY);
    bj_window_update_framebuffer(data->window);

    bj_sleep(15);

    return bj_window_should_close(data->window) 
         ? bj_callback_exit_success 
         : bj_callback_continue;
}

int bj_app_end(void* user_data, int status) {
    game_data* data = (game_data*)user_data;

    bj_bitmap_del(data->drawbuffer);
    bj_window_del(data->window);
    bj_end(0);
    return status;
}
