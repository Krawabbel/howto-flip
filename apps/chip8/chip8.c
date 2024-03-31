// /*

// This is an example of using a ViewPort.  It is a simple Hello World program that
// allows you to move a cursor around the screen with the arrow keys.  Pressing
// the back button will exit the program.

// Uncomment the different view_port_set_orientation() calls to see how the
// orientation of the screen and keypad change.

// The code is from the Message Queue wiki page
// (https://github.com/jamisonderek/flipper-zero-tutorials/wiki/Message-Queue) and
// also the ViewPort section of the User Interface wiki page
// (https://github.com/jamisonderek/flipper-zero-tutorials/wiki/User-Interface#viewport).

// */

// #include <furi.h>
// #include <furi_hal.h>
// #include <gui/gui.h>
// #include <gui/view.h>
// #include <gui/view_dispatcher.h>
// #include <storage/storage.h>
// #include <toolbox/stream/stream.h>
// #include <toolbox/stream/file_stream.h>

// #include "chip8_icons.h"

// #include "vm.h"

// typedef enum {
//     MyEventTypeDone,
// } MyEventType;

// typedef struct {
//     MyEventType type; // The reason for this event.
//     InputEvent input; // This data is specific to keypress data.
// } MyEvent;

// // FuriMessageQueue* queue;

// // VM* vm;

// // static void my_draw_callback(Canvas* canvas, void* context) {
// //     UNUSED(context);
// //     canvas_clear(canvas);
// //     // canvas_set_font(canvas, FontPrimary);
// //     // canvas_draw_str(canvas, 5, 30, "Chip-8");

// //     const uint8_t x_orig = (canvas_width(canvas) - SCREEN_WIDTH) / 2;
// //     const uint8_t y_orig = (canvas_height(canvas) - SCREEN_HEIGHT) / 2;

// //     for(uint8_t x_line = 0; x_line < SCREEN_WIDTH; x_line++) {
// //         for(uint8_t y_line = 0; y_line < SCREEN_HEIGHT; y_line++) {
// //             const uint8_t x = x_orig + x_line;
// //             const uint8_t y = y_orig + y_line;
// //             if(vm->screen[x_line][y_line]) canvas_draw_dot(canvas, x, y);
// //         }
// //     }

// //     if(play_sound(vm)) {
// //         if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1)) {
// //             furi_hal_speaker_start(440.0f, 0.5f);
// //         }
// //     } else {
// //         if(furi_hal_speaker_is_mine()) {
// //             furi_hal_speaker_stop();
// //             furi_hal_speaker_release();
// //         }
// //     }
// // }

// // static void my_input_callback(InputEvent* input_event, void* context) {
// //     UNUSED(context);

// //     switch(input_event->key) {
// //     case InputKeyBack:
// //         MyEvent event;
// //         event.type = MyEventTypeDone;
// //         furi_message_queue_put(queue, &event, FuriWaitForever);
// //         break;
// //     case InputKeyLeft:
// //     case InputKeyRight:
// //     case InputKeyUp:
// //     case InputKeyDown:
// //     case InputKeyOk:
// //         for(size_t key = 0; key < 0x10; key++) {
// //             vm->is_key_pressed[key] = input_event->type == InputTypeShort;
// //         }
// //     default:
// //         break;
// //     }
// // }

// // static void my_debug_callback(void* context) {
// //     UNUSED(context);
// //     if(vm == NULL) return;
// //     const uint32_t cpu_time_s = (furi_get_tick() - vm->init_timestamp) / 1000;
// //     const uint32_t cpu_speed = cpu_time_s > 0 ? vm->cpu_ticks / cpu_time_s : 0;
// //     FURI_LOG_D("chip8", "[INFO] cpu speed: %lu Hz", cpu_speed);
// // }

// // static void my_update_callback(void* context) {
// //     UNUSED(context);
// //     if(vm == NULL) return;
// //     const Chip8Status status = update(vm, furi_get_tick());
// //     if(status != Chip8StatusOK) {
// //         FURI_LOG_D("chip8", "[ERROR] code %d", status);
// //         MyEvent event;
// //         event.type = MyEventTypeDone;
// //         furi_message_queue_put(queue, &event, FuriWaitForever);
// //     }
// // }

// // void close_ch8() {
// //     free(vm);
// // }

// // void load_ch8() {
// //     vm = (VM*)malloc(sizeof(VM));
// //     if(vm == NULL) {
// //         FURI_LOG_D("chip8", "failed to allocate vm");
// //     }
// //     init(vm, furi_get_tick());

// //     Storage* storage = furi_record_open(RECORD_STORAGE);
// //     if(storage == NULL) {
// //         FURI_LOG_D("chip8", "failed to open storage");
// //     }

// //     storage_simply_mkdir(storage, ROM_PATH);

// //     File* dir = storage_file_alloc(storage);
// //     storage_dir_open(dir, ROM_PATH);

// //     FileInfo fileinfo;
// //     FURI_LOG_D("chip8", "assets:");
// //     char name[100];
// //     FuriString* path = furi_string_alloc();
// //     while(storage_dir_read(dir, &fileinfo, name, 100)) {
// //         FURI_LOG_D("chip8", "%s", name);

// //         furi_string_set_str(path, ROM_PATH);
// //         furi_string_push_back(path, '/');
// //         furi_string_cat(path, name);
// //     };
// //     storage_file_free(dir);

// //     FURI_LOG_D("chip8", "%s", name);

// //     Stream* stream = file_stream_alloc(storage);
// //     if(stream == NULL) {
// //         FURI_LOG_D("chip8", "failed to open stream");
// //     }

// //     FuriString* line = furi_string_alloc();

// //     FURI_LOG_D("chip8", "----------------------------------------");
// //     FURI_LOG_D("chip8", "File \"%s\" content:", furi_string_get_cstr(path));

// //     uint8_t c[1];
// //     if(file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
// //         for(size_t pos = 0; !stream_eof(stream); pos++) {
// //             if(stream_read(stream, c, 1)) {
// //                 vm->memory[PROG_START + pos] = *c;
// //                 FURI_LOG_T("chip8", "%d: %u", pos, *c);
// //             } else {
// //                 FURI_LOG_D("chip8", "failed to read next byte");
// //             }
// //         }
// //     } else {
// //         FURI_LOG_D("chip8", "Failed to open file with code %d", file_stream_get_error(stream));
// //     }
// //     FURI_LOG_D("chip8", "----------------------------------------");

// //     furi_string_free(path);
// //     furi_string_free(line);
// //     file_stream_close(stream);
// //     stream_free(stream);
// //     furi_record_close(RECORD_STORAGE);
// // }

// typedef enum {
//     MyFileBrowserViewId,
//     MyEmulatorViewId,
//     MyConfigViewId,
// } MyViewIds;

// bool my_view_dispatcher_navigation_event_callback(void* context) {
//     ViewDispatcher* view_dispatcher = context;
//     view_dispatcher_switch_to_view(view_dispatcher, MyFileBrowserViewId);
//     return true;
// }

// int32_t chip8_app(void* p) {
//     UNUSED(p);
//     ViewDispatcher* view_dispatcher = view_dispatcher_alloc();

//     // For this demo, we just use view_dispatcher as our application context.
//     void* my_context = view_dispatcher;

//     FuriString* path = furi_string_alloc();
//     FileBrowser* browser = file_browser_alloc(path);
//     file_browser_configure(browser, "*", ROM_PATH, true, true, &I_dolphin, true);

//     View* browser_view = file_browser_get_view(browser);

//     // View* view2 = view_alloc();
//     // view_set_context(view2, my_context);
//     // view_set_draw_callback(view2, my_draw_callback);
//     // view_set_input_callback(view2, my_input_callback);
//     // view_set_orientation(view2, ViewOrientationVertical);

//     view_dispatcher_set_event_callback_context(view_dispatcher, my_context);
//     view_dispatcher_set_navigation_event_callback(
//         view_dispatcher, my_view_dispatcher_navigation_event_callback);
//     view_dispatcher_enable_queue(view_dispatcher);

//     view_dispatcher_add_view(view_dispatcher, MyFileBrowserViewId, browser_view);
//     // view_dispatcher_add_view(view_dispatcher, MyEmulatorViewId, view2);

//     Gui* gui = furi_record_open(RECORD_GUI);
//     view_dispatcher_attach_to_gui(view_dispatcher, gui, ViewDispatcherTypeFullscreen);
//     view_dispatcher_switch_to_view(view_dispatcher, MyFileBrowserViewId);
//     view_dispatcher_run(view_dispatcher);

//     view_dispatcher_remove_view(view_dispatcher, MyFileBrowserViewId);
//     // view_dispatcher_remove_view(view_dispatcher, MyEmulatorViewId);
//     furi_record_close(RECORD_GUI);

//     file_browser_free(browser);
//     furi_string_free(path);

//     view_dispatcher_free(view_dispatcher);
//     return 0;
// }

// // int32_t chip8_app_backup(void* p) {
// //     UNUSED(p);
// //     FURI_LOG_D("chip8", "[INFO] starting chip-8 emulator");

// //     ViewDispatcher* view_dispatcher = view_dispatcher_alloc();

// //     load_ch8();

// //     queue = furi_message_queue_alloc(8, sizeof(MyEvent));
// //     if(queue == NULL) {
// //         FURI_LOG_D("chip8", "failed to allocate queue");
// //     }

// //     void* my_context = NULL;

// //     FuriTimer* debug_timer =
// //         furi_timer_alloc(my_debug_callback, FuriTimerTypePeriodic, my_context);
// //     if(debug_timer == NULL) {
// //         FURI_LOG_D("chip8", "failed to allocate debug_timer");
// //     }

// //     FuriTimer* update_timer =
// //         furi_timer_alloc(my_update_callback, FuriTimerTypePeriodic, my_context);
// //     if(update_timer == NULL) {
// //         FURI_LOG_D("chip8", "failed to allocate update_timer");
// //     }

// //     View* view_main = view_alloc();
// //     if(view_main == NULL) {
// //         FURI_LOG_D("chip8", "failed to allocate view_main");
// //     }

// //     view_port_draw_callback_set(view_main, my_draw_callback, my_context);
// //     view_port_input_callback_set(view_main, my_input_callback, my_context);
// //     view_port_set_orientation(view_main, ViewPortOrientationHorizontal);

// //     Gui* gui = furi_record_open(RECORD_GUI);
// //     if(gui == NULL) {
// //         FURI_LOG_D("chip8", "failed to open gui");
// //     }

// //     gui_add_view_port(gui, view_port, GuiLayerFullscreen);

// //     furi_timer_start(debug_timer, furi_kernel_get_tick_frequency());
// //     furi_timer_start(update_timer, furi_kernel_get_tick_frequency() / CPU_TICKS_PER_SEC);

// //     MyEvent event;
// //     bool keep_processing = true;
// //     while(keep_processing) {
// //         if(furi_message_queue_get(queue, &event, FuriWaitForever) == FuriStatusOk) {
// //             if(event.type == MyEventTypeDone) {
// //                 keep_processing = false;
// //             }
// //         } else {
// //             keep_processing = false;
// //         }
// //     }

// //     FURI_LOG_D("chip8", "[INFO] stopping chip-8 emulator");

// //     furi_message_queue_free(queue);
// //     furi_timer_free(debug_timer);
// //     furi_timer_free(update_timer);
// //     view_port_enabled_set(view_port, false);
// //     gui_remove_view_port(gui, view_port);
// //     furi_record_close(RECORD_GUI);
// //     view_port_free(view_port);

// //     close_ch8();

// //     view_dispatcher_free(view_dispatcher);

// //     return 0;
// // }

/*

This is an example of using a ViewDispatcher.  It is a simple Hello World program that
allows you to move a cursor around the screen with the arrow keys.  Pressing the OK button
will switch between two Views (the views share the same callback functions), just have 
different screen orientations.  Pressing the the back button will exit the program.

The code is from the Message Queue wiki page 
(https://github.com/jamisonderek/flipper-zero-tutorials/wiki/Message-Queue) and
also the ViewDispatcher section of the User Interface wiki page
(https://github.com/jamisonderek/flipper-zero-tutorials/wiki/User-Interface#viewdispatcher).

*/

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <gui/modules/file_browser.h>

#include "vm.h"

#include "chip8_icons.h"

#define ROM_PATH (EXT_PATH("chip8"))

typedef enum {
    MyEventTypeKey,
    MyEventTypeDone,
} MyEventType;

typedef struct {
    MyEventType type; // The reason for this event.
    InputEvent input; // This data is specific to keypress data.
} MyEvent;

typedef enum {
    MyViewId,
    MyOtherViewId,
} ViewId;

FuriMessageQueue* queue;
int x = 32;
int y = 48;
ViewId current_view;

static void my_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 30, "Hello world");
    canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, "^");
}

static bool my_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    bool handled = false;
    // we set our callback context to be the view_dispatcher.
    ViewDispatcher* view_dispatcher = context;

    if(input_event->type == InputTypeShort) {
        if(input_event->key == InputKeyBack) {
            // Default back handler.
            handled = false;
        } else if(input_event->key == InputKeyLeft) {
            x--;
            handled = true;
        } else if(input_event->key == InputKeyRight) {
            x++;
            handled = true;
        } else if(input_event->key == InputKeyUp) {
            y--;
            handled = true;
        } else if(input_event->key == InputKeyDown) {
            y++;
            handled = true;
        } else if(input_event->key == InputKeyOk) {
            // switch the view!
            view_dispatcher_send_custom_event(view_dispatcher, 42);
            handled = true;
        }
    }

    return handled;
}

bool view_dispatcher_navigation_event_callback(void* context) {
    UNUSED(context);
    // We did not handle the event, so return false.
    return false;
}

bool my_view_dispatcher_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    bool handled = false;
    // we set our callback context to be the view_dispatcher.
    ViewDispatcher* view_dispatcher = context;

    if(event == 42) {
        if(current_view == MyViewId) {
            current_view = MyOtherViewId;
        } else {
            current_view = MyViewId;
        }

        view_dispatcher_switch_to_view(view_dispatcher, current_view);
        handled = true;
    }

    // NOTE: The return value is not currently used by the ViewDispatcher.
    return handled;
}

int32_t chip8_app() {
    ViewDispatcher* view_dispatcher = view_dispatcher_alloc();

    // For this demo, we just use view_dispatcher as our application context.
    void* my_context = view_dispatcher;

    View* view1 = view_alloc();
    view_set_context(view1, my_context);
    view_set_draw_callback(view1, my_draw_callback);
    view_set_input_callback(view1, my_input_callback);
    view_set_orientation(view1, ViewOrientationHorizontal);

    FuriString* path = furi_string_alloc_set_str(ROM_PATH);
    FileBrowser* browser = file_browser_alloc(path);
    file_browser_configure(browser, "ch8", ROM_PATH, true, true, NULL, true);
    file_browser_start(browser, path);

    View* view2 = file_browser_get_view(browser);

    // set param 1 of custom event callback (impacts tick and navigation too).
    view_dispatcher_set_event_callback_context(view_dispatcher, my_context);
    view_dispatcher_set_navigation_event_callback(
        view_dispatcher, view_dispatcher_navigation_event_callback);
    view_dispatcher_set_custom_event_callback(
        view_dispatcher, my_view_dispatcher_custom_event_callback);
    view_dispatcher_enable_queue(view_dispatcher);
    view_dispatcher_add_view(view_dispatcher, MyViewId, view1);
    view_dispatcher_add_view(view_dispatcher, MyOtherViewId, view2);

    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    current_view = MyViewId;
    view_dispatcher_switch_to_view(view_dispatcher, current_view);
    view_dispatcher_run(view_dispatcher);

    view_dispatcher_remove_view(view_dispatcher, MyViewId);
    view_dispatcher_remove_view(view_dispatcher, MyOtherViewId);
    furi_record_close(RECORD_GUI);
    view_dispatcher_free(view_dispatcher);
    return 0;
}
