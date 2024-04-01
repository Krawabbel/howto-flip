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

// #include "chip8_icons.h"

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

#include "game.h"

#include "chip8_icons.h"

#define ROM_PATH (EXT_PATH("chip8"))

typedef struct Chip8App {
    ViewDispatcher* view_dispatcher;
    FuriString* path;
    FileBrowser* browser;
    Game* game;
} Chip8;

typedef enum {
    FileBrowserViewId,
    GameViewId,
} ViewId;

bool view_dispatcher_navigation_event_callback(void* context) {
    UNUSED(context);
    // We did not handle the event, so return false.
    return false;
}

bool my_view_dispatcher_custom_event_callback(void* context, uint32_t event) {
    UNUSED(context);
    UNUSED(event);
    bool handled = false;
    // we set our callback context to be the view_dispatcher.
    // ViewDispatcher* view_dispatcher = context;

    // if(event == 42) {
    //     if(current_view == MyViewId) {
    //         current_view = MyOtherViewId;
    //     } else {
    //         current_view = MyViewId;
    //     }

    //     view_dispatcher_switch_to_view(view_dispatcher, current_view);
    //     handled = true;
    // }

    // NOTE: The return value is not currently used by the ViewDispatcher.
    return handled;
}

static void file_browser_callback(void* context) {
    Chip8* chip8 = context;
    FURI_LOG_D("chip8", "selected file '%s'", furi_string_get_cstr(chip8->path));
    game_start(chip8->game, chip8->path);
    view_dispatcher_switch_to_view(chip8->view_dispatcher, GameViewId);
}

int32_t chip8_app() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, ROM_PATH);
    furi_record_close(RECORD_STORAGE);

    Chip8* chip8 = malloc(sizeof(Chip8));
    chip8->view_dispatcher = view_dispatcher_alloc();
    chip8->path = furi_string_alloc_set_str(ROM_PATH);
    chip8->browser = file_browser_alloc(chip8->path);
    chip8->game = game_alloc();

    void* context = chip8;

    file_browser_set_callback(chip8->browser, file_browser_callback, context);
    file_browser_configure(chip8->browser, "ch8", NULL, true, true, NULL, true);
    file_browser_start(chip8->browser, chip8->path);
    View* browser_view = file_browser_get_view(chip8->browser);
    view_dispatcher_add_view(chip8->view_dispatcher, FileBrowserViewId, browser_view);

    View* game_view = game_get_view(chip8->game);
    view_dispatcher_add_view(chip8->view_dispatcher, GameViewId, game_view);

    view_dispatcher_set_event_callback_context(chip8->view_dispatcher, context);
    view_dispatcher_set_navigation_event_callback(
        chip8->view_dispatcher, view_dispatcher_navigation_event_callback);
    view_dispatcher_set_custom_event_callback(
        chip8->view_dispatcher, my_view_dispatcher_custom_event_callback);

    view_dispatcher_enable_queue(chip8->view_dispatcher);

    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(chip8->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_switch_to_view(chip8->view_dispatcher, FileBrowserViewId);
    view_dispatcher_run(chip8->view_dispatcher);

    view_dispatcher_remove_view(chip8->view_dispatcher, FileBrowserViewId);
    view_dispatcher_remove_view(chip8->view_dispatcher, GameViewId);

    furi_record_close(RECORD_GUI);

    game_free(chip8->game);
    file_browser_free(chip8->browser);
    furi_string_free(chip8->path);
    view_dispatcher_free(chip8->view_dispatcher);

    return 0;
}
