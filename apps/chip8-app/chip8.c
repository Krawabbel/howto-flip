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
    file_browser_configure(chip8->browser, "ch8", ROM_PATH, true, true, &I_chip8, true);
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
    view_dispatcher_free(chip8->view_dispatcher);

    game_free(chip8->game);

    furi_string_free(chip8->path);
    file_browser_free(chip8->browser);

    return 0;
}
