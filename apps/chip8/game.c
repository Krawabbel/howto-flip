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
    byte screen[VM_SCREEN_WIDTH][VM_SCREEN_HEIGHT];
    bool sound;
} GameData;

typedef struct Chip8Game {
    VM* vm;
    View* view;
    FuriTimer* update_timer;
    FuriTimer* debug_timer;
    GameData* data;
} Game;

static void game_draw_callback(Canvas* canvas, void* model) {
    furi_check(model, "game_draw_callback");
    GameData* data = model;

    canvas_clear(canvas);

    const uint8_t x_orig = (canvas_width(canvas) - VM_SCREEN_WIDTH) / 2;
    const uint8_t y_orig = (canvas_height(canvas) - VM_SCREEN_HEIGHT) / 2;

    for(uint8_t x_line = 0; x_line < VM_SCREEN_WIDTH; x_line++) {
        for(uint8_t y_line = 0; y_line < VM_SCREEN_HEIGHT; y_line++) {
            const uint8_t x = x_orig + x_line;
            const uint8_t y = y_orig + y_line;
            if(data->screen[x_line][y_line]) canvas_draw_dot(canvas, x, y);
        }
    }

    if(data->sound) {
        if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
            furi_hal_speaker_start(440.0f, 0.5f);
        }
    } else {
        if(furi_hal_speaker_is_mine()) {
            furi_hal_speaker_stop();
            furi_hal_speaker_release();
        }
    }
}

static bool game_input_callback(InputEvent* input_event, void* context) {
    furi_check(context, "game_input_callback");
    Game* game = context;

    // ViewDispatcher* view_dispatcher = context;
    // view_dispatcher_send_custom_event(view_dispatcher, 42);

    if(input_event->type == InputTypeShort) {
        switch(input_event->key) {
        case InputKeyLeft:
            return true;
        case InputKeyRight:
            return true;
        case InputKeyUp:
            return true;
        case InputKeyDown:
            return true;
        case InputKeyOk:
            for(size_t key = 0; key < VM_NUM_KEYS; key++) {
                vm_set_key_input(game->vm, key, true);
            }
            return true;
        case InputKeyBack:
            return false;
        default:
            return false;
        }
    }
    return false;
}

static void game_debug_callback(void* context) {
    furi_check(context, "game_debug_callback");
    Game* game = context;
    FURI_LOG_D("chip8", "cpu speed: %lu Hz", vm_speed(game->vm, furi_get_tick()));
}

static void game_update_callback(void* context) {
    furi_check(context, "game_update_callback");
    Game* game = context;
    if(!vm_update(game->vm, furi_get_tick())) {
        furi_crash("update error");
    }

    with_view_model(
        game->view,
        GameData * data,
        {
            data->sound = vm_is_sound_playing(game->vm);
            for(size_t x = 0; x < VM_SCREEN_WIDTH; x++) {
                for(size_t y = 0; y < VM_SCREEN_HEIGHT; y++) {
                    data->screen[x][y] = vm_get_pixel(game->vm, x, y);
                }
            }
        },
        true);
}

Game* game_alloc() {
    Game* game = malloc(sizeof(Game));
    void* context = game;

    game->vm = vm_alloc();
    game->view = view_alloc();
    game->debug_timer = furi_timer_alloc(game_debug_callback, FuriTimerTypePeriodic, context);
    game->update_timer = furi_timer_alloc(game_update_callback, FuriTimerTypePeriodic, context);

    view_allocate_model(game->view, ViewModelTypeLocking, sizeof(GameData));

    view_set_context(game->view, context);
    view_set_draw_callback(game->view, game_draw_callback);
    view_set_input_callback(game->view, game_input_callback);

    return game;
}

void game_free(Game* game) {
    view_free_model(game->view);
    furi_timer_free(game->update_timer);
    furi_timer_free(game->debug_timer);
    vm_free(game->vm);
    view_free(game->view);
    free(game);
}

View* game_get_view(Game* game) {
    return game->view;
}

void game_load(Game* game, FuriString* path) {
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
        vm_write_prog_to_memory(game->vm, addr, *c);
        FURI_LOG_D("chip8", "%d: %u", addr, *c);
    }

    furi_string_free(line);
    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

void game_start(Game* game, FuriString* path) {
    game_load(game, path);
    vm_start(game->vm, furi_get_tick());
    furi_timer_start(game->debug_timer, furi_kernel_get_tick_frequency());
    furi_timer_start(game->update_timer, furi_kernel_get_tick_frequency() / VM_TICKS_PER_SEC);
}