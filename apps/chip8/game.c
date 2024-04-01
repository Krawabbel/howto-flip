#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#include "game.h"

#include "vm.h"

typedef struct Chip8GameData {
    VM* vm;
} GameData;

typedef struct Chip8Game {
    View* view;
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

    if(vm_is_sound_playing(data->vm)) {
        if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
            furi_hal_speaker_start(880.0f, 0.5f);
        }
    } else {
        if(furi_hal_speaker_is_mine()) {
            furi_hal_speaker_stop();
            furi_hal_speaker_release();
        }
    }
}

static size_t map_input_to_key_id(InputEvent* input_event) {
    switch(input_event->key) {
    case InputKeyLeft:
        return 0x07;
    case InputKeyRight:
        return 0x09;
    case InputKeyUp:
        return 0x05;
    case InputKeyDown:
        return 0x08;
    default:
        break;
    }
    furi_crash("unexpected input");
    return -1;
}

static uint16_t modify_keys(InputEvent* input_event, const word keys) {
    if(input_event->key == InputKeyOk) {
        return 0x00;
    }
    const size_t id = map_input_to_key_id(input_event);

    const bool is_key_pressed = (input_event->type == InputTypePress);
    if(is_key_pressed) {
        return keys | (1 << id);
    }
    return keys & (~(1 << id));
}

static bool game_input_callback(InputEvent* input_event, void* context) {
    furi_check(context, "game_input_callback");
    Game* game = context;

    with_view_model(
        game->view, GameData * data, { game_data_update(data); }, false);

    FURI_LOG_D(
        "chip8",
        "input %s %s",
        input_get_key_name(input_event->key),
        input_get_type_name(input_event->type));

    if(input_event->key == InputKeyBack) {
        if(furi_hal_speaker_is_mine()) {
            furi_hal_speaker_stop();
            furi_hal_speaker_release();
        }
        return false;
    }

    with_view_model(
        game->view,
        GameData * data,
        {
            if(!vm_update(data->vm, furi_get_tick())) {
                furi_crash("update error");
            }
            const word keys = modify_keys(input_event, vm_get_keys(data->vm));
            vm_set_keys(data->vm, keys);
        },
        false);

    return true;
}

Game* game_alloc() {
    Game* game = malloc(sizeof(Game));

    game->view = view_alloc();

    view_allocate_model(game->view, ViewModelTypeLocking, sizeof(GameData));
    with_view_model(
        game->view, GameData * data, { data->vm = vm_alloc(); }, false);

    void* context = game;
    view_set_context(game->view, context);

    view_set_draw_callback(game->view, game_draw_callback);
    view_set_input_callback(game->view, game_input_callback);

    return game;
}

void game_free(Game* game) {
    with_view_model(
        game->view, GameData * data, { vm_free(data->vm); }, false);
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

void game_start(Game* game, FuriString* path) {
    with_view_model(
        game->view,
        GameData * data,
        {
            game_data_load(data, path);
            vm_start(data->vm, furi_get_tick());
        },
        false);
}