#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <core/thread.h>

#include "game.h"

#include "vm.h"

typedef enum {
    SoundThreadFlagExit = 0x10,
} SoundThreadFlag;

typedef struct Chip8GameData {
    VM* vm;
    bool input_to_key_map[InputKeyMAX][VM_NUM_KEYS];
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
        vm_cpu_speed(data->vm, furi_get_tick()),
        vm_timer_speed(data->vm, furi_get_tick()));
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

    const uint8_t x_orig = (canvas_width(canvas) - VM_SCREEN_WIDTH) / 2;
    const uint8_t y_orig = (canvas_height(canvas) - VM_SCREEN_HEIGHT) / 2;

    for(uint8_t x_line = 0; x_line < VM_SCREEN_WIDTH; x_line++) {
        for(uint8_t y_line = 0; y_line < VM_SCREEN_HEIGHT; y_line++) {
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
                        furi_hal_speaker_start(440.F, 0.5F);
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

static uint16_t game_data_map_input_to_keys(GameData* data, InputEvent* input_event) {
    const bool is_key_pressed = (input_event->type == InputTypePress);
    const size_t input_id = (size_t)(input_event->key);
    uint16_t keys = 0x00;
    for(size_t key_id = 0; key_id < VM_NUM_KEYS; key_id++) {
        const int bit = is_key_pressed && data->input_to_key_map[input_id][key_id];
        keys |= (bit << key_id);
    }
    return keys;
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
            const word key_bitfield = game_data_map_input_to_keys(data, input_event);
            vm_set_keys(data->vm, key_bitfield);
        },
        false);

    return true;
}

static void game_enter_callback(void* context) {
    UNUSED(context);
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
        furi_hal_speaker_start(440.0f, 0.5f);
        furi_delay_ms(100);
        furi_hal_speaker_start(550.0f, 0.5f);
        furi_delay_ms(100);
        furi_hal_speaker_start(660.0f, 0.5f);
        furi_delay_ms(100);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

static void game_exit_callback(void* context) {
    UNUSED(context);
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
        furi_hal_speaker_start(660.0f, 0.5f);
        furi_delay_ms(100);
        furi_hal_speaker_start(550.0f, 0.5f);
        furi_delay_ms(100);
        furi_hal_speaker_start(440.0f, 0.5f);
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
        game->view, GameData * data, { vm_free(data->vm); }, false);
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
    FuriString* line = furi_string_alloc();

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

    furi_string_free(line);
    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

static void game_data_connect_input_to_key(GameData* data, InputKey input_key, size_t key_id) {
    const size_t input_id = input_key;
    data->input_to_key_map[input_id][key_id] = true;
}

void game_start(Game* game, FuriString* path) {
    with_view_model(
        game->view,
        GameData * data,
        {
            game_data_load(data, path);

            for(size_t input_id = 0; input_id < InputKeyMAX; input_id++) {
                for(size_t key_id = 0; key_id < VM_NUM_KEYS; key_id++) {
                    data->input_to_key_map[input_id][key_id] = false;
                }
            }

            game_data_connect_input_to_key(data, InputKeyUp, 0x02);
            game_data_connect_input_to_key(data, InputKeyDown, 0x08);
            game_data_connect_input_to_key(data, InputKeyOk, 0x05);

            vm_start(data->vm, furi_get_tick());
        },
        false);

    furi_thread_start(game->sound_thread);
}