/*

This is an example of using a ViewPort.  It is a simple Hello World program that
allows you to move a cursor around the screen with the arrow keys.  Pressing
the back button will exit the program.

Uncomment the different view_port_set_orientation() calls to see how the
orientation of the screen and keypad change.

The code is from the Message Queue wiki page 
(https://github.com/jamisonderek/flipper-zero-tutorials/wiki/Message-Queue) and
also the ViewPort section of the User Interface wiki page
(https://github.com/jamisonderek/flipper-zero-tutorials/wiki/User-Interface#viewport).

*/

#include <furi.h>
#include <gui/gui.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#include "vm.h"

typedef enum {
    MyEventTypeDone,
} MyEventType;

typedef struct {
    MyEventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
} MyEvent;

FuriMessageQueue* queue;

VM* vm;

static void my_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    canvas_clear(canvas);
    // canvas_set_font(canvas, FontPrimary);
    // canvas_draw_str(canvas, 5, 30, "Chip-8");

    const uint8_t x_orig = (canvas_width(canvas) - SCREEN_WIDTH) / 2;
    const uint8_t y_orig = (canvas_height(canvas) - SCREEN_HEIGHT) / 2;

    for(uint8_t x_line = 0; x_line < SCREEN_WIDTH; x_line++) {
        for(uint8_t y_line = 0; y_line < SCREEN_HEIGHT; y_line++) {
            const uint8_t x = x_orig + x_line;
            const uint8_t y = y_orig + y_line;
            if(vm->screen[x_line][y_line]) canvas_draw_dot(canvas, x, y);
        }
    }
}

static void my_input_callback(InputEvent* input_event, void* context) {
    UNUSED(context);

    switch(input_event->key) {
    case InputKeyBack:
        MyEvent event;
        event.type = MyEventTypeDone;
        furi_message_queue_put(queue, &event, FuriWaitForever);
        break;
    case InputKeyLeft:
    case InputKeyRight:
    case InputKeyUp:
    case InputKeyDown:
    case InputKeyOk:
    default:
        break;
    }
}

static void my_debug_callback(void* context) {
    UNUSED(context);
    if(vm == NULL) return;
    const uint32_t cpu_time_s = (furi_get_tick() - vm->init_timestamp) / 1000;
    const uint32_t cpu_speed = cpu_time_s > 0 ? vm->cpu_ticks / cpu_time_s : 0;
    FURI_LOG_D("chip8", "[INFO] cpu speed: %lu Hz", cpu_speed);
}

static void my_update_callback(void* context) {
    UNUSED(context);
    if(vm == NULL) return;
    const Chip8Status status = update(vm, furi_get_tick());
    if(status != Chip8StatusOK) {
        FURI_LOG_D("chip8", "[ERROR] code %d", status);
        MyEvent event;
        event.type = MyEventTypeDone;
        furi_message_queue_put(queue, &event, FuriWaitForever);
    }
}

int32_t chip8_app(void* p) {
    UNUSED(p);
    FURI_LOG_D("chip8", "[INFO] starting chip-8 emulator");

    vm = (VM*)malloc(sizeof(VM));
    if(vm == NULL) {
        FURI_LOG_D("chip8", "failed to allocate vm");
    }
    init(vm, furi_get_tick());

// #define path APP_ASSETS_PATH("heart_monitor.txt")
#define path APP_ASSETS_PATH("corax89.txt")

    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(storage == NULL) {
        FURI_LOG_D("chip8", "failed to open storage");
    }

    Stream* stream = file_stream_alloc(storage);
    if(stream == NULL) {
        FURI_LOG_D("chip8", "failed to open stream");
    }

    FuriString* line = furi_string_alloc();
    if(line == NULL) {
        FURI_LOG_D("chip8", "failed to open line");
    }

    FURI_LOG_D("chip8", "----------------------------------------");
    FURI_LOG_D("chip8", "File \"%s\" content:", path);
    uint8_t c[1];
    if(file_stream_open(stream, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        for(size_t pos = 0; !stream_eof(stream); pos++) {
            if(stream_read(stream, c, 1)) {
                vm->memory[PROG_START + pos] = *c;
                FURI_LOG_D("chip8", "%d: %u", pos, *c);
            } else {
                FURI_LOG_D("chip8", "failed to read next byte");
            }
        }
    } else {
        FURI_LOG_D("chip8", "Failed to open file with code %d", file_stream_get_error(stream));
    }
    FURI_LOG_D("chip8", "----------------------------------------");

    furi_string_free(line);
    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    void* my_context = NULL;
    queue = furi_message_queue_alloc(8, sizeof(MyEvent));
    if(queue == NULL) {
        FURI_LOG_D("chip8", "failed to allocate queue");
    }

    FuriTimer* debug_timer =
        furi_timer_alloc(my_debug_callback, FuriTimerTypePeriodic, my_context);
    if(debug_timer == NULL) {
        FURI_LOG_D("chip8", "failed to allocate debug_timer");
    }

    FuriTimer* update_timer =
        furi_timer_alloc(my_update_callback, FuriTimerTypePeriodic, my_context);
    if(update_timer == NULL) {
        FURI_LOG_D("chip8", "failed to allocate update_timer");
    }

    ViewPort* view_port = view_port_alloc();
    if(view_port == NULL) {
        FURI_LOG_D("chip8", "failed to allocate view_port");
    }

    view_port_draw_callback_set(view_port, my_draw_callback, my_context);
    view_port_input_callback_set(view_port, my_input_callback, my_context);
    view_port_set_orientation(view_port, ViewPortOrientationHorizontal);

    Gui* gui = furi_record_open(RECORD_GUI);
    if(gui == NULL) {
        FURI_LOG_D("chip8", "failed to open gui");
    }

    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    furi_timer_start(debug_timer, furi_kernel_get_tick_frequency());
    furi_timer_start(update_timer, furi_kernel_get_tick_frequency() / CPU_TICKS_PER_SEC);

    MyEvent event;
    bool keep_processing = true;
    while(keep_processing) {
        if(furi_message_queue_get(queue, &event, FuriWaitForever) == FuriStatusOk) {
            if(event.type == MyEventTypeDone) {
                keep_processing = false;
            }
        } else {
            keep_processing = false;
        }
    }

    FURI_LOG_D("chip8", "[INFO] stopping chip-8 emulator");

    furi_message_queue_free(queue);
    furi_timer_free(debug_timer);
    furi_timer_free(update_timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    free(vm);

    return 0;
}
