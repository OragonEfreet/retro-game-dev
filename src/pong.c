////////////////////////////////////////////////////////////////////////////////
/// \example pong.c
/// A clone of Pong.
////////////////////////////////////////////////////////////////////////////////
#define BJ_AUTOMAIN_CALLBACKS
#include <banjo/assert.h>
#include <banjo/bitmap.h>
#include <banjo/event.h>
#include <banjo/linmath.h>
#include <banjo/log.h>
#include <banjo/main.h>
#include <banjo/shader.h>
#include <banjo/system.h>
#include <banjo/time.h>
#include <banjo/window.h>

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Compile time, customize before building.
#define SCREEN_W 800
#define SCREEN_H 600
#define BALL_SIZE 16
#define PADDLE_MARGIN 50
#define PADDLE_LENGTH 120
#define PADDLE_WIDTH 24
#define PADDLE_VELOCITY (250.f)
#define GAME_START_DELAY (1.f)

// Computed from above, but you can tweak them if you want
#define LEFT_PADDLE_POSX (PADDLE_MARGIN)
#define RIGHT_PADDLE_POSX (SCREEN_W - PADDLE_MARGIN - PADDLE_WIDTH)
#define MIDDLE_BLOCK       (SCREEN_H/12)
#define MIDDLE_DASH_LENGTH (MIDDLE_BLOCK*2/3)
#define MIDDLE_GAP_LENGTH  (MIDDLE_BLOCK/3)
#define MIDDLE_THICKNESS   ((SCREEN_W)/400 + 1)
#define MIDDLE_DASH_COUNT  ((SCREEN_H + MIDDLE_GAP_LENGTH) / MIDDLE_BLOCK)
#define MIDDLE_PATTERN_LEN (MIDDLE_DASH_COUNT * MIDDLE_DASH_LENGTH + (MIDDLE_DASH_COUNT - 1) * MIDDLE_GAP_LENGTH)
#define MIDDLE_START_Y     ((SCREEN_H - MIDDLE_PATTERN_LEN) / 2)
#define MIDDLE_START_X      ((SCREEN_W   - MIDDLE_THICKNESS) / 2)

// Don't cange this
#define CHAR_PIXEL_WIDTH 5
#define CHAR_PIXEL_HEIGHT 7

#define CHAR_DISPLAY_WIDTH (CHAR_PIXEL_WIDTH * 10)
#define CHAR_DISPLAY_HEIGHT (CHAR_PIXEL_HEIGHT * 10)
#define CHAR_DISPLAY_SPACING 10
#define SCORE_LEFT_X (SCREEN_W / 4)
#define SCORE_RIGHT_X ((SCREEN_W / 4) * 3)
#define SCORE_Y (CHAR_DISPLAY_SPACING + CHAR_DISPLAY_HEIGHT / 2)

////////////////////////////////////////////////////////////////////////////////
// Game data
static struct {
    struct {
        bj_vec2 position;
        bj_vec2 velocity;
    } ball;

    struct {
        float   position_y;
        bj_bool up;
        bj_bool down;
        unsigned short score;
    } paddle[2];
    bj_bool running;
} game = {0};

bj_window* window         = 0;
bj_bitmap* framebuffer    = 0;
bj_bitmap* charset_buffer = 0;
bj_stopwatch stopwatch    = {0};

////////////////////////////////////////////////////////////////////////////////
// Draw the scene
// Important: bj_bitmap_draw_rect draws from the top-left corner of the rect
// while the game's entities (paddles, ball) have coordinates on their centers.
void draw(bj_bitmap* framebuffer) {
    bj_bitmap_clear(framebuffer);

    const uint32_t color = bj_bitmap_pixel_value(framebuffer, 0xFF, 0xFF, 0xFF);

    // Middle line
    for (int i = 0; i < MIDDLE_DASH_COUNT; ++i) {
        bj_rect dash = {
            .x = MIDDLE_START_X,
            .y = MIDDLE_START_Y + i * MIDDLE_BLOCK,
            .w = MIDDLE_THICKNESS,
            .h = MIDDLE_DASH_LENGTH
        };
        bj_bitmap_draw_rectangle(framebuffer, &dash, color);
    }
    
    // Ball
    static bj_rect ball_rect = { .w = BALL_SIZE, .h = BALL_SIZE,};
    ball_rect.x = game.ball.position[0] - BALL_SIZE / 2;
    ball_rect.y = game.ball.position[1] - BALL_SIZE / 2;
    bj_bitmap_draw_rectangle(framebuffer, &ball_rect, color);

    static bj_rect paddle_rect[2] = {
        {.x = LEFT_PADDLE_POSX, .w = PADDLE_WIDTH, .h = PADDLE_LENGTH,},
        {.x = RIGHT_PADDLE_POSX, .w = PADDLE_WIDTH, .h = PADDLE_LENGTH,},
    };

    for(size_t r = 0 ; r < 2 ; ++r) {
        paddle_rect[r].y = game.paddle[r].position_y - PADDLE_LENGTH / 2;
        bj_bitmap_draw_rectangle(framebuffer, &paddle_rect[r], color);
    }

    /* bj_bitmap_blit_stretched(charset_buffer, 0, framebuffer, &(bj_rect) { */
    /*     0, 0, CHAR_DISPLAY_WIDTH * 10, CHAR_DISPLAY_HEIGHT */
    /* }); */

    // Text
    for(size_t r = 0 ; r < 2 ; ++r) {
        const size_t n_digits = game.paddle[r].score < 10 ? 1 : 2;
        int char_x = (r == 0 ? SCORE_LEFT_X : SCORE_RIGHT_X);
        if(n_digits == 2) {
            char_x += CHAR_DISPLAY_SPACING / 2;
        } else {
            char_x -= CHAR_DISPLAY_WIDTH / 2;
        }

        for(size_t d = 0 ; d < n_digits ; ++d) {
            const size_t digit = game.paddle[r].score / (int)pow(10.0, d) % 10;
            const bj_rect origin = {
                .x = CHAR_PIXEL_WIDTH * digit, .y = 0, .w = CHAR_PIXEL_WIDTH, .h = CHAR_PIXEL_HEIGHT
            };
            const bj_rect dest = {
                .x = char_x,
                .y = SCORE_Y - CHAR_DISPLAY_HEIGHT / 2,
                .w = CHAR_DISPLAY_WIDTH,
                .h = CHAR_DISPLAY_HEIGHT,
            };
            bj_bitmap_blit_stretched(
                charset_buffer, &origin,
                framebuffer, &dest
            );
            char_x -= (CHAR_DISPLAY_SPACING + CHAR_DISPLAY_WIDTH);
        }
    }
}

void key_callback(bj_window* p_window, const bj_key_event* e) {
    (void)p_window;

    static const struct {
        bj_key up;
        bj_key down;
    } keymap[2] = {
        {.up = BJ_KEY_D, .down = BJ_KEY_F},
        {.up = BJ_KEY_UP, .down = BJ_KEY_DOWN},
    };

    for(size_t r = 0 ; r < 2 ; ++r) {
        if(e->key == keymap[r].up) {
            game.paddle[r].up = e->action == BJ_PRESS;
        }
        if(e->key == keymap[r].down) {
            game.paddle[r].down = e->action == BJ_PRESS;
        }
    }

    if(e->key == BJ_KEY_ESCAPE && e->action == BJ_RELEASE) {
        bj_window_set_should_close(p_window);
    } else {
        game.running = BJ_TRUE;
    }
}

static void reset_game() {
    bj_vec2_set(game.ball.position, SCREEN_W / 2, SCREEN_H / 2);
    bj_vec2_set(game.ball.velocity, 200.f, 200.f);

    for(size_t p = 0 ; p < 2 ; ++p) {
        game.paddle[p].position_y = SCREEN_H / 2;
        game.paddle[p].up         = BJ_FALSE;
        game.paddle[p].down       = BJ_FALSE;
    }
    
    game.running = BJ_FALSE;
}

static void prepare_text(bj_pixel_mode mode) {
    static uint8_t digit_lines[10][7] = {
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E,}, // 0
        {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E,}, // 1
        {0x0E, 0x11, 0x02, 0x04, 0x08, 0x10, 0x1F,}, // 2
        {0x0E, 0x11, 0x01, 0x0E, 0x01, 0x11, 0x0E,}, // 3
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02,}, // 4
        {0x1F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E,}, // 5
        {0x0E, 0x11, 0x10, 0x1E, 0x11, 0x11, 0x0E,}, // 6
        {0x1F, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04,}, // 7
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E,}, // 8
        {0x0E, 0x11, 0x11, 0x0E, 0x01, 0x11, 0x0E,}, // 9
    };

    bj_assert(framebuffer);

    charset_buffer = bj_bitmap_new(
        CHAR_PIXEL_WIDTH * 10, CHAR_PIXEL_HEIGHT,
        mode, 0
    );

    const uint32_t color    = bj_bitmap_pixel_value(charset_buffer, 0xFF, 0xFF, 0xFF);

    for(size_t digit = 0 ; digit < 10 ; ++digit) {
        for(size_t row = 0 ; row < 7 ; ++row) {
            for(size_t column = 0 ; column < 5 ; ++column) {
                if(((digit_lines[digit][row] >> column) & 0x01) > 0) {
                    bj_bitmap_put_pixel(
                        charset_buffer, digit * 5 + (4 - column),
                        row, color
                    );
                }

            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Game update
void update(double dt) {
    const float halfB = BALL_SIZE * 0.5f;
    bj_vec2 tp;


    if (game.running) {
        // Ball
        bj_vec2_add_scaled(tp, game.ball.position, game.ball.velocity, dt);

        if (tp[1] < halfB || tp[1] > SCREEN_H - halfB) {
            tp[1] = bj_clamp(tp[1], halfB, SCREEN_H - halfB); game.ball.velocity[1] = -game.ball.velocity[1];
        }

        float speed = bj_vec2_len(game.ball.velocity);
        for (int p = 0; p < 2; p++) {
            float cx = (p == 0 ? LEFT_PADDLE_POSX : RIGHT_PADDLE_POSX) + PADDLE_WIDTH * 0.5f;
            if (fabsf(tp[0] - cx) <= halfB + PADDLE_WIDTH * 0.5f &&
                fabsf(tp[1] - game.paddle[p].position_y) <= halfB + PADDLE_LENGTH * 0.5f) 
            {
                // random angle away from paddle: base 0 (left) or π (right) ±45°
                float base = p == 0 ? 0.0f : BJ_PI;
                float randn = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                float angle = base + randn * (BJ_PI / 4.0f);
                game.ball.velocity[0] = cosf(angle) * speed;
                game.ball.velocity[1] = sinf(angle) * speed;
                break;
            }
        }

        bj_vec2_copy(game.ball.position, tp);

    }

    if(game.ball.position[0] < -(BALL_SIZE / 2)) {
        bj_info("Right Score: %d", ++game.paddle[1].score);
        reset_game();
    }
    if(game.ball.position[0] > SCREEN_W + BALL_SIZE / 2) {
        bj_info("Left Score: %d", ++game.paddle[0].score);
        reset_game();
    }

    // paddle movement
    for (int p = 0; p < 2; p++) {
        float dir = (float)game.paddle[p].down - (float)game.paddle[p].up;
        float y  = game.paddle[p].position_y + dir * dt * PADDLE_VELOCITY;
        game.paddle[p].position_y = bj_clamp(y, PADDLE_LENGTH * 0.5f, SCREEN_H - PADDLE_LENGTH * 0.5f);
    }
}

int bj_app_begin(void** user_data, int argc, char* argv[]) {
    (void)user_data; (void)argc; (void)argv;

    bj_error* p_error = 0;

    if(!bj_begin(&p_error)) {
        bj_err("Error 0x%08X: %s", p_error->code, p_error->message);
        return bj_callback_exit_error;
    } 


    window      = bj_window_new("Pong", 0,0, SCREEN_W, SCREEN_H, 0);
    framebuffer = bj_window_get_framebuffer(window, 0);

    prepare_text(bj_bitmap_mode(framebuffer));

    bj_set_key_callback(key_callback);

    reset_game();
    return bj_callback_continue;
}

int bj_app_iterate(void* user_data) {
    (void)user_data;

    bj_dispatch_events();
    update(bj_stopwatch_step_delay(&stopwatch));

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
    bj_bitmap_del(charset_buffer);
    bj_end(0);
    return status;
}

