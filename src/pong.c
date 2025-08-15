#define BJ_AUTOMAIN_CALLBACKS
#include <banjo/assert.h>
#include <banjo/audio.h>
#include <banjo/bitmap.h>
#include <banjo/draw.h>
#include <banjo/event.h>
#include <banjo/linmath.h>
#include <banjo/log.h>
#include <banjo/main.h>
#include <banjo/shader.h>
#include <banjo/system.h>
#include <banjo/time.h>
#include <banjo/window.h>

#include <math.h>
#include <stdlib.h>

#define SCREEN_W 800
#define SCREEN_H 600
#define BALL_SIZE 16
#define PADDLE_MARGIN 50
#define PADDLE_LENGTH 120
#define PADDLE_WIDTH 24
#define PADDLE_VELOCITY (250.f)
#define GAME_START_DELAY (1.f)

#define BALL_INIT_SPEED   (282.842712f)
#define BALL_SPEED_GAIN   (1.01f)
#define BALL_CAP_SPEED    (BALL_INIT_SPEED * 3.0f)

#define LEFT_PADDLE_POSX   (PADDLE_MARGIN)
#define RIGHT_PADDLE_POSX  (SCREEN_W - PADDLE_MARGIN - PADDLE_WIDTH)
#define MIDDLE_BLOCK       (SCREEN_H/12)
#define MIDDLE_DASH_LENGTH (MIDDLE_BLOCK*2/3)
#define MIDDLE_GAP_LENGTH  (MIDDLE_BLOCK/3)
#define MIDDLE_THICKNESS   ((SCREEN_W)/400 + 1)
#define MIDDLE_DASH_COUNT  ((SCREEN_H + MIDDLE_GAP_LENGTH) / MIDDLE_BLOCK)
#define MIDDLE_PATTERN_LEN (MIDDLE_DASH_COUNT * MIDDLE_DASH_LENGTH + (MIDDLE_DASH_COUNT - 1) * MIDDLE_GAP_LENGTH)
#define MIDDLE_START_Y     ((SCREEN_H - MIDDLE_PATTERN_LEN) / 2)
#define MIDDLE_START_X     ((SCREEN_W   - MIDDLE_THICKNESS) / 2)

#define CHAR_PIXEL_WIDTH 5
#define CHAR_PIXEL_HEIGHT 7

#define CHAR_DISPLAY_WIDTH (CHAR_PIXEL_WIDTH * 10)
#define CHAR_DISPLAY_HEIGHT (CHAR_PIXEL_HEIGHT * 10)
#define CHAR_DISPLAY_SPACING 10
#define SCORE_LEFT_X (SCREEN_W / 4)
#define SCORE_RIGHT_X ((SCREEN_W / 4) * 3)
#define SCORE_Y (CHAR_DISPLAY_SPACING + CHAR_DISPLAY_HEIGHT / 2)

#define AUDIO_BUZZ_FREQUENCY 286.94

static struct {
    struct {
        bj_vec2 position;
        float   speed;
        float   angle;
    } ball;

    struct {
        float   position_y;
        bj_bool up;
        bj_bool down;
        unsigned short score;
    } paddle[2];

    bj_bool running;
    bj_bool game_over;
} game = {0};

bj_audio_device* audio_device = 0;
bj_bitmap* charset_buffer     = 0;
bj_bitmap* framebuffer        = 0;
bj_stopwatch stopwatch        = {0};
bj_window* window             = 0;

bj_audio_play_note_data audio = {.function = BJ_AUDIO_PLAY_SQUARE, .frequency = AUDIO_BUZZ_FREQUENCY};
bj_stopwatch audio_stopwatch  = {0};
double audio_length           = 0.5f;

static float normalize_angle(float a) {
    a = fmodf(a + BJ_PI, 2.0f * BJ_PI);
    if (a < 0) a += 2.0f * BJ_PI;
    return a - BJ_PI;
}

static void play_sound(bj_bool high, double duration) {
    bj_stopwatch_reset(&audio_stopwatch);
    audio.frequency = high ? AUDIO_BUZZ_FREQUENCY * 2.0 : AUDIO_BUZZ_FREQUENCY;
    audio_length = duration;
    bj_audio_device_play(audio_device);
}

static void update_audio() {
    bj_stopwatch_step(&audio_stopwatch);
    double e = bj_stopwatch_elapsed(&audio_stopwatch);
    if(e >= audio_length) {
        bj_audio_device_stop(audio_device);
    }
}

static void draw_score_and_lines(bj_bitmap* framebuffer) {
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
                framebuffer, &dest,
                BJ_BLIT_OP_COPY
            );
            char_x -= (CHAR_DISPLAY_SPACING + CHAR_DISPLAY_WIDTH);
        }
    }

    const uint32_t color = bj_bitmap_pixel_value(framebuffer, 0xFF, 0xFF, 0xFF);
    for (int i = 0; i < MIDDLE_DASH_COUNT; ++i) {
        bj_rect dash = {
            .x = MIDDLE_START_X,
            .y = MIDDLE_START_Y + i * MIDDLE_BLOCK,
            .w = MIDDLE_THICKNESS,
            .h = MIDDLE_DASH_LENGTH
        };
        bj_bitmap_draw_filled_rectangle(framebuffer, &dash, color);
    }
}

void draw(bj_bitmap* framebuffer) {
    bj_bitmap_clear(framebuffer);
    draw_score_and_lines(framebuffer);

    if (game.game_over)
        return;

    const uint32_t color = bj_bitmap_pixel_value(framebuffer, 0xFF, 0xFF, 0xFF);

    static bj_rect ball_rect = { .w = BALL_SIZE, .h = BALL_SIZE,};
    ball_rect.x = game.ball.position[0] - BALL_SIZE / 2;
    ball_rect.y = game.ball.position[1] - BALL_SIZE / 2;
    bj_bitmap_draw_filled_rectangle(framebuffer, &ball_rect, color);

    static bj_rect paddle_rect[2] = {
        {.x = LEFT_PADDLE_POSX, .w = PADDLE_WIDTH, .h = PADDLE_LENGTH,},
        {.x = RIGHT_PADDLE_POSX, .w = PADDLE_WIDTH, .h = PADDLE_LENGTH,},
    };
    for(size_t r = 0 ; r < 2 ; ++r) {
        paddle_rect[r].y = game.paddle[r].position_y - PADDLE_LENGTH / 2;
        bj_bitmap_draw_filled_rectangle(framebuffer, &paddle_rect[r], color);
    }
}

static void reset_game() {
    bj_vec2_set(game.ball.position, SCREEN_W / 2, SCREEN_H / 2);
    game.ball.speed = BALL_INIT_SPEED;

    float side = (rand() & 1) ? 0.0f : BJ_PI;
    float randn = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    game.ball.angle = side + randn * (BJ_PI * 0.25f);

    for(size_t p = 0 ; p < 2 ; ++p) {
        game.paddle[p].position_y = SCREEN_H / 2;
        game.paddle[p].up         = BJ_FALSE;
        game.paddle[p].down       = BJ_FALSE;
    }
    
    game.running = BJ_FALSE;
    play_sound(0, 1.0);
}


void key_callback(bj_window* p_window, const bj_key_event* e, void* data) {
    (void)p_window; (void)data;

    static const struct {
        bj_key up[2];
        bj_key down[2];
    } keymap[2] = {
        {.up = {BJ_KEY_Z, BJ_KEY_W}, .down = {BJ_KEY_S, BJ_KEY_S}},
        {.up = {BJ_KEY_K, BJ_KEY_UP}, .down = {BJ_KEY_J, BJ_KEY_DOWN}},
    };

    for(size_t r = 0 ; r < 2 ; ++r) {
        for(size_t k = 0 ; k < 2 ; ++k) {
            if(e->key == keymap[r].up[k]) {
                game.paddle[r].up = e->action == BJ_PRESS;
                game.running = BJ_TRUE;
            }
            if(e->key == keymap[r].down[k]) {
                game.paddle[r].down = e->action == BJ_PRESS;
                game.running = BJ_TRUE;
            }
        }
    }

    if(e->key == BJ_KEY_ESCAPE && e->action == BJ_PRESS) {
        if(game.running) {
            game.paddle[0].score = 0;
            game.paddle[1].score = 0;
            game.game_over = BJ_FALSE;
            reset_game();
        } else {
            bj_window_set_should_close(window);
        }
    }
}

static void prepare_text(bj_pixel_mode mode) {
    static uint8_t digit_lines[10][7] = {
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E,},
        {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E,},
        {0x0E, 0x11, 0x02, 0x04, 0x08, 0x10, 0x1F,},
        {0x0E, 0x11, 0x01, 0x0E, 0x01, 0x11, 0x0E,},
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02,},
        {0x1F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E,},
        {0x0E, 0x11, 0x10, 0x1E, 0x11, 0x11, 0x0E,},
        {0x1F, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04,},
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E,},
        {0x0E, 0x11, 0x11, 0x0E, 0x01, 0x11, 0x0E,},
    };

    charset_buffer = bj_bitmap_new(
        CHAR_PIXEL_WIDTH * 10, CHAR_PIXEL_HEIGHT,
        mode, 0
    );

    const uint32_t color = bj_bitmap_pixel_value(charset_buffer, 0xFF, 0xFF, 0xFF);

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

static void check_game_over() {
    if (game.paddle[0].score >= 15 || game.paddle[1].score >= 15 ||
        abs((int)game.paddle[0].score - (int)game.paddle[1].score) >= 2) {
        game.game_over = BJ_TRUE;
        game.running = BJ_FALSE;
    }
}

static void game_logic(double dt) {
    if (game.game_over)
        return;

    const float halfB = BALL_SIZE * 0.5f;

    if (game.running) {
        float vx = cosf(game.ball.angle) * game.ball.speed;
        float vy = sinf(game.ball.angle) * game.ball.speed;

        float nx = game.ball.position[0] + vx * (float)dt;
        float ny = game.ball.position[1] + vy * (float)dt;

        if (ny < halfB || ny > SCREEN_H - halfB) {
            ny = bj_clamp(ny, halfB, SCREEN_H - halfB);
            game.ball.angle = normalize_angle(-game.ball.angle);
            play_sound(1, 0.1);
        }

        for (int p = 0; p < 2; ++p) {
            float cx = (p == 0 ? LEFT_PADDLE_POSX : RIGHT_PADDLE_POSX) + PADDLE_WIDTH * 0.5f;
            if (bj_fabsf(nx - cx) <= halfB + PADDLE_WIDTH * 0.5f &&
                bj_fabsf(ny - game.paddle[p].position_y) <= halfB + PADDLE_LENGTH * 0.5f)
            {
                play_sound(1, 0.1);
                float base  = (p == 0) ? 0.0f : BJ_PI;
                float randn = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                float angle = base + randn * (BJ_PI * 0.25f);
                float impact = (ny - game.paddle[p].position_y) / (PADDLE_LENGTH * 0.5f);
                angle += impact * (BJ_PI * 0.10f);
                game.ball.angle = normalize_angle(angle);

                nx = (p == 0)
                    ? cx + (halfB + PADDLE_WIDTH * 0.5f) + 0.5f
                    : cx - (halfB + PADDLE_WIDTH * 0.5f) - 0.5f;

                game.ball.speed *= BALL_SPEED_GAIN;
                if (game.ball.speed > BALL_CAP_SPEED)
                    game.ball.speed = BALL_CAP_SPEED;

                break;
            }
        }

        game.ball.position[0] = nx;
        game.ball.position[1] = ny;
    }

    if(game.ball.position[0] < -(BALL_SIZE / 2)) {
        ++game.paddle[1].score;
        check_game_over();
        reset_game();
    }
    if(game.ball.position[0] > SCREEN_W + BALL_SIZE / 2) {
        ++game.paddle[0].score;
        check_game_over();
        reset_game();
    }

    for (int p = 0; p < 2; p++) {
        float dir = (float)game.paddle[p].down - (float)game.paddle[p].up;
        float y  = game.paddle[p].position_y + dir * (float)dt * PADDLE_VELOCITY;
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

    bj_set_key_callback(key_callback, 0);

    audio_device = bj_open_audio_device(&(bj_audio_properties){
        .format      = BJ_AUDIO_FORMAT_F32,
        .amplitude   = 16000,
        .sample_rate = 44100,
        .channels    = 2,
    }, bj_audio_play_note, &audio, &p_error);

    if (audio_device == 0) {
        if (p_error) {
            bj_err("cannot open audio: %s (%x)", p_error->message, p_error->code);
        }
        return bj_callback_exit_error;
    }

    reset_game();
    return bj_callback_continue;
}

int bj_app_iterate(void* user_data) {
    (void)user_data;

    bj_dispatch_events();
    game_logic(bj_stopwatch_step_delay(&stopwatch));
    update_audio();

    draw(framebuffer);
    bj_window_update_framebuffer(window);
    bj_sleep(15);

    return bj_window_should_close(window) 
         ? bj_callback_exit_success 
         : bj_callback_continue;
}

int bj_app_end(void* user_data, int status) {
    (void)user_data;
    bj_close_audio_device(audio_device);
    bj_window_del(window);
    bj_bitmap_del(charset_buffer);
    bj_end(0);
    return status;
}
