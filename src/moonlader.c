#define BJ_AUTOMAIN_CALLBACKS
#include <banjo/assert.h>
#include <banjo/audio.h>
#include <banjo/bitmap.h>
#include <banjo/event.h>
#include <banjo/linmath.h>
#include <banjo/log.h>
#include <banjo/main.h>
#include <banjo/shader.h>
#include <banjo/system.h>
#include <banjo/time.h>
#include <banjo/window.h>

#define SCREEN_W 800
#define SCREEN_H 600

bj_bitmap* framebuffer        = 0;
bj_stopwatch stopwatch        = {0};
bj_window* window             = 0;

void draw(bj_bitmap* framebuffer) {
    bj_bitmap_clear(framebuffer);

}

void key_callback(bj_window* p_window, const bj_key_event* e) {
    (void)p_window;

    if(e->key == BJ_KEY_ESCAPE && e->action == BJ_RELEASE) {
        bj_window_set_should_close(window);
    }
}

static void game_logic(double dt) {
    (void)dt;

}

int bj_app_begin(void** user_data, int argc, char* argv[]) {
    (void)user_data; (void)argc; (void)argv;

    bj_error* p_error = 0;
    if(!bj_begin(&p_error)) {
        bj_err("Error 0x%08X: %s", p_error->code, p_error->message);
        return bj_callback_exit_error;
    } 

    window      = bj_window_new("Moonlander", 0,0, SCREEN_W, SCREEN_H, 0);
    framebuffer = bj_window_get_framebuffer(window, 0);

    bj_set_key_callback(key_callback);

    return bj_callback_continue;
}

int bj_app_iterate(void* user_data) {
    (void)user_data;

    bj_dispatch_events();
    game_logic(bj_stopwatch_step_delay(&stopwatch));

    draw(framebuffer);
    bj_window_update_framebuffer(window);
    bj_sleep(15);

    return bj_window_should_close(window) 
         ? bj_callback_exit_success 
         : bj_callback_continue;
}

int bj_app_end(void* user_data, int status) {
    (void)user_data;
    bj_window_del(window);
    bj_end(0);
    return status;
}
