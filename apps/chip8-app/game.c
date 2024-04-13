#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <core/thread.h>

#include "game.h"

#include "button_config.h"
#include "vm.h"

#define BEEP_VOLUME 0.5F

typedef enum {
    SoundThreadFlagExit = 0x10,
} SoundThreadFlag;

typedef struct Chip8GameData {
    VM* vm;
    ButtonConfig* button_config;
    FuriThreadId sound_thread_id;
} GameData;

typedef struct Chip8Game {
    View* view;
    FuriThread* sound_thread;
} Game;

static void game_data_update(GameData* data) {
    FURI_LOG_D(
        "chip8",
        "update: cpu speed = %lu Hz / timer freq = %lu Hz",
        vm_calc_cpu_speed(data->vm, furi_get_tick()),
        vm_calc_timer_speed(data->vm, furi_get_tick()));
    if(!vm_update(data->vm, furi_get_tick())) {
        furi_crash("update error");
    }
}

static void game_draw_callback(Canvas* canvas, void* model) {
    furi_check(model, "game_draw_callback");
    GameData* data = model;

    game_data_update(data);

    FURI_LOG_D("chip8", "draw");
    canvas_clear(canvas);

    const int screen_width = vm_get_screen_width(data->vm);
    const int screen_height = vm_get_screen_height(data->vm);

    const uint8_t x_orig = (canvas_width(canvas) - screen_width) / 2;
    const uint8_t y_orig = (canvas_height(canvas) - screen_height) / 2;

    for(uint8_t x_line = 0; x_line < screen_width; x_line++) {
        for(uint8_t y_line = 0; y_line < screen_height; y_line++) {
            const uint8_t x = x_orig + x_line;
            const uint8_t y = y_orig + y_line;
            if(vm_get_pixel(data->vm, x_line, y_line)) canvas_draw_dot(canvas, x, y);
        }
    }
}

/* This sound functionality runs in a separate thread. */
static int32_t sound_thread_callback(void* context) {
    FURI_LOG_D("chip8", "starting sound");
    Game* game = context;
    for(;;) {
        if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
            with_view_model(
                game->view,
                GameData * data,
                {
                    if(vm_is_sound_playing(data->vm)) {
                        furi_hal_speaker_start(880.F, BEEP_VOLUME);
                    } else {
                        furi_hal_speaker_stop();
                    }
                },
                false);
        } else {
            FURI_LOG_D("chip8", "could not acquire speaker");
        }

        /* Give the sound some time to play. At the same time wait for an exit signal. */
        const int flags = furi_thread_flags_wait(SoundThreadFlagExit, FuriFlagWaitAny, 10);
        FURI_LOG_D("chip8", "sound thread received flag %d", flags);

        /* If an exit signal was received, return from this thread. */
        if(flags != FuriStatusErrorTimeout) {
            FURI_LOG_D("chip8", "stopping sound");
            break;
        }
    }

    furi_hal_speaker_release();
    return 0;
}

static void game_end(Game* game) {
    /* Signal the sound thread to cease operation and exit */
    furi_thread_flags_set(furi_thread_get_id(game->sound_thread), SoundThreadFlagExit);
    furi_thread_join(game->sound_thread);
}

static bool game_input_callback(InputEvent* input_event, void* context) {
    furi_check(context, "game_input_callback");
    Game* game = context;

    FURI_LOG_D(
        "chip8",
        "input %s %s",
        input_get_key_name(input_event->key),
        input_get_type_name(input_event->type));

    if(input_event->key == InputKeyBack) {
        game_end(game);
        return false;
    }

    with_view_model(
        game->view,
        GameData * data,
        {
            game_data_update(data);
            const word key_bitfield =
                button_config_map_input_to_keys(data->button_config, input_event);
            vm_set_keys(data->vm, key_bitfield);
        },
        false);

    return true;
}

static void game_enter_callback(void* context) {
    UNUSED(context);
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
        furi_hal_speaker_start(440.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_start(550.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_start(660.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

static void game_exit_callback(void* context) {
    UNUSED(context);
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
        furi_hal_speaker_start(660.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_start(550.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_start(440.0f, BEEP_VOLUME);
        furi_delay_ms(100);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

Game* game_alloc() {
    Game* game = malloc(sizeof(Game));
    void* context = game;

    game->sound_thread =
        furi_thread_alloc_ex("sound thread", 1024U, sound_thread_callback, context);

    game->view = view_alloc();

    view_allocate_model(game->view, ViewModelTypeLocking, sizeof(GameData));
    with_view_model(
        game->view,
        GameData * data,
        {
            data->vm = vm_alloc();
            data->button_config = button_config_alloc(BUTTON_CONFIG_PATH);
            data->sound_thread_id = furi_thread_get_id(game->sound_thread);
        },
        false);

    view_set_context(game->view, context);

    view_set_draw_callback(game->view, game_draw_callback);
    view_set_input_callback(game->view, game_input_callback);
    view_set_enter_callback(game->view, game_enter_callback);
    view_set_exit_callback(game->view, game_exit_callback);

    return game;
}

void game_free(Game* game) {
    with_view_model(
        game->view,
        GameData * data,
        {
            button_config_free(data->button_config, BUTTON_CONFIG_PATH);
            vm_free(data->vm);
        },
        false);
    furi_thread_free(game->sound_thread);
    view_free(game->view);
    free(game);
}

View* game_get_view(Game* game) {
    return game->view;
}

static void game_data_load(GameData* data, FuriString* path) {
    FURI_LOG_D("chip8", "loading file '%s'", furi_string_get_cstr(path));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    FURI_LOG_D("chip8", "----------------------------------------");
    FURI_LOG_D("chip8", "bytecode:");

    uint8_t c[1];
    furi_check(
        file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING));

    for(size_t addr = 0; !stream_eof(stream); addr++) {
        furi_check(stream_read(stream, c, 1));
        vm_write_prog_to_memory(data->vm, addr, *c);
        FURI_LOG_D("chip8", "%d: %u", addr, *c);
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

void game_start(Game* game, FuriString* path) {
    with_view_model(
        game->view,
        GameData * data,
        {
            game_data_load(data, path);
            vm_start(data->vm, furi_get_tick());
        },
        false);

    furi_thread_start(game->sound_thread);
}